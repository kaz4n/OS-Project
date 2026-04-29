#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include "shell.h"
#include "scheduler.h"

#define PORT 8080
#define BUFFER_SIZE 1024

/* Global job queue */
static JobQueue *global_queue = NULL;
static pthread_mutex_t session_id_mutex = PTHREAD_MUTEX_INITIALIZER;
static int next_session_id = 1;

static int send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

/**
 * Execute command in child process and send output + scheduler metadata to client
 * Tracks timing information: wait_ms (time from submission to execution start),
 * runtime_ms (time from execution start to completion), and quantum_ms (time quantum)
 */
static int execute_and_send_with_scheduler(int client_fd, SchedulerJob *job) {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) != 0) {
        perror("pipe");
        return 0;
    }

    /* Record when execution starts */
    struct timespec exec_start;
    clock_gettime(CLOCK_MONOTONIC, &exec_start);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]); close(pipefd[1]);
        return 0;
    }

    if (pid == 0) {
        /* child: redirect stdout/stderr to pipe and run command */
        close(pipefd[0]);
        if (job->cwd[0] != '\0' && chdir(job->cwd) != 0) {
            perror("chdir");
            _exit(1);
        }
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(1);
        if (dup2(pipefd[1], STDERR_FILENO) < 0) _exit(1);
        close(pipefd[1]);
        process_input(job->command);
        fflush(stdout); fflush(stderr);
        _exit(0);
    }

    /* parent: read from pipe and forward to client socket */
    close(pipefd[1]);
    char buf[BUFFER_SIZE];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        if (!send_all(client_fd, buf, (size_t)n)) break;
    }
    close(pipefd[0]);

    /* wait for child to finish */
    int status = 0;
    waitpid(pid, &status, 0);

    /* Record when execution ends */
    struct timespec exec_end;
    clock_gettime(CLOCK_MONOTONIC, &exec_end);

    /* Calculate timing metrics */
    long wait_ms = get_elapsed_ms(job->submit_time, exec_start);
    long runtime_ms = get_elapsed_ms(exec_start, exec_end);

    /* Send scheduler metadata to client */
    char scheduler_info[256];
    snprintf(scheduler_info, sizeof(scheduler_info),
             "\n[scheduler] job=%d wait_ms=%ld runtime_ms=%ld quantum_ms=%d exit=%d\n",
             job->job_id, wait_ms, runtime_ms, TIME_QUANTUM_MS, WEXITSTATUS(status));

    send_all(client_fd, scheduler_info, strlen(scheduler_info));

    return 1;
}

/**
 * Dispatcher thread: processes jobs from the queue and executes them
 * One dispatcher thread runs continuously, taking jobs from queue and executing them
 */
void* dispatcher_thread(void *arg) {
    SchedulerJob job;

    while (1) {
        /* Dequeue a job (blocks if queue is empty) */
        if (!queue_dequeue(global_queue, &job)) {
            continue;
        }

        /* Execute the job with scheduler timing */
        if (!execute_and_send_with_scheduler(job.client_fd, &job)) {
            const char *err = "server: failed to execute command\n";
            send_all(job.client_fd, err, strlen(err));
        }

        /* Note: socket remains open for persistent connection */
    }

    return NULL;
}

int setup_server_socket(int port) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 16) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

int accept_client(int server_fd) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);

    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd < 0) {
        perror("Accept failed");
        return -1;
    }

    return client_fd;
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    int n;
    ShellSession session;

    pthread_mutex_lock(&session_id_mutex);
    int session_id = next_session_id++;
    pthread_mutex_unlock(&session_id_mutex);

    shell_session_init(&session, client_fd, session_id);
    shell_session_set_current(&session);
    
    /* Keep connection alive and process multiple commands until client exits */
    while (1) {
        memset(buffer, 0, sizeof(buffer));

        n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            /* Client disconnected */
            break;
        }

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        char parse_buffer[BUFFER_SIZE];
        strncpy(parse_buffer, buffer, sizeof(parse_buffer) - 1);
        parse_buffer[sizeof(parse_buffer) - 1] = '\0';

        Pipeline pipeline;
        if (parse_input(parse_buffer, &pipeline) && pipeline.num_commands == 1) {
            char *cmd = pipeline.commands[0].argv[0];

            if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "cd_new") == 0) {
                handle_cd_session(&pipeline.commands[0], &session);
                continue;
            }

            if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "exit_new") == 0) {
                break;
            }
        }

        /* Enqueue job to scheduler */
        int job_id = queue_enqueue(global_queue, buffer, client_fd, session.cwd, session.session_id);
        if (job_id < 0) {
            const char *err = "server: job queue full\n";
            send_all(client_fd, err, strlen(err));
        }
    }

    shell_session_set_current(NULL);
    close(client_fd);
}

void *client_thread(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    handle_client(client_fd);

    return NULL;
}

int main() {
    /* Initialize the job queue for the scheduler */
    global_queue = queue_init();
    if (!global_queue) {
        fprintf(stderr, "Failed to initialize job queue\n");
        return EXIT_FAILURE;
    }

    /* Create dispatcher thread to process jobs from the queue */
    pthread_t dispatcher_tid;
    if (pthread_create(&dispatcher_tid, NULL, dispatcher_thread, NULL) != 0) {
        perror("pthread_create dispatcher failed");
        queue_destroy(global_queue);
        return EXIT_FAILURE;
    }

    /* Dispatcher thread runs indefinitely */
    pthread_detach(dispatcher_tid);

    int server_fd = setup_server_socket(PORT);
    printf("Server listening on port %d\n", PORT);
    printf("Dispatcher thread started for job scheduling\n");

    while (1) {
        int client_fd = accept_client(server_fd);

        if (client_fd < 0) {
            continue;
        }

        printf("New client connected\n");

        pthread_t tid;
        int *pclient = malloc(sizeof(int));

        if (pclient == NULL) {
            perror("malloc failed");
            close(client_fd);
            continue;
        }

        *pclient = client_fd;

        if (pthread_create(&tid, NULL, client_thread, pclient) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(pclient);
            continue;
        }

        pthread_detach(tid);
    }

    close(server_fd);
    queue_destroy(global_queue);
    return 0;
}
