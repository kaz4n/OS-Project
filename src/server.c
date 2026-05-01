#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include "shell.h"
#include "scheduler.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define END_OF_OUTPUT_MARKER "\n<<END_OF_OUTPUT>>\n"

typedef struct {
    int client_fd;
    int session_id;
    char ip[INET_ADDRSTRLEN];
    int port;
} ClientContext;

/* Global job queue */
static JobQueue *global_queue = NULL;
static pthread_mutex_t session_id_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static int next_session_id = 1;

static void log_printf(const char *fmt, ...) {
    va_list args;
    pthread_mutex_lock(&log_mutex);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
    pthread_mutex_unlock(&log_mutex);
}

static int send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

static void send_end_marker(int fd) {
    send_all(fd, END_OF_OUTPUT_MARKER, strlen(END_OF_OUTPUT_MARKER));
}

/* forward declaration to avoid implicit declaration in job_worker */
static int execute_and_send_with_scheduler(int client_fd, SchedulerJob *job);

/* Worker thread that executes a single job concurrently */
static void *job_worker(void *arg) {
    SchedulerJob *job = (SchedulerJob *)arg;
    if (!job) return NULL;

    if (!execute_and_send_with_scheduler(job->client_fd, job)) {
        const char *err = "server: failed to execute command\n";
        send_all(job->client_fd, err, strlen(err));
        send_end_marker(job->client_fd);
    }

    free(job);
    return NULL;
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
    send_end_marker(client_fd);

    return 1;
}

/**
  Dispatcher thread: processes jobs from the queue and executes them
  One dispatcher thread runs continuously, taking jobs from queue and executing them
 */
void* dispatcher_thread(void *arg) {
    (void)arg; /* silence unused parameter warning */
    SchedulerJob job;

    while (1) {
        /* Dequeue a job (blocks if queue is empty) */
        if (!queue_dequeue(global_queue, &job)) {
            continue;
        }

        /* Run each job in its own detached thread so jobs execute concurrently */
        SchedulerJob *job_copy = malloc(sizeof(SchedulerJob));
        if (job_copy == NULL) {
            const char *err = "server: failed to allocate job worker\n";
            send_all(job.client_fd, err, strlen(err));
            send_end_marker(job.client_fd);
            continue;
        }
        *job_copy = job;

        pthread_t worker_tid;
        if (pthread_create(&worker_tid, NULL, job_worker, job_copy) != 0) {
            const char *err = "server: failed to create job worker thread\n";
            send_all(job.client_fd, err, strlen(err));
            send_end_marker(job.client_fd);
            free(job_copy);
            continue;
        }
        pthread_detach(worker_tid);

        /* Note: socket remains open for persistent connection; job runs concurrently */
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

void handle_client(ClientContext *ctx) {
    char buffer[BUFFER_SIZE];
    int n;
    ShellSession session;
    int client_fd = ctx->client_fd;

    shell_session_init(&session, client_fd, ctx->session_id);
    shell_session_set_current(&session);

    log_printf("Client %d connected from %s:%d\n", ctx->session_id, ctx->ip, ctx->port);

    {
        char welcome[256];
        snprintf(welcome, sizeof(welcome),
                 "Welcome to remote shell. Your Client ID is %d.\n",
                 ctx->session_id);
        send_all(client_fd, welcome, strlen(welcome));
        send_end_marker(client_fd);
    }
    
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

        /* Log client command on server stdout */
        log_printf("Server Output: [Client %d] Command: %s\n", session.session_id, buffer);

        char parse_buffer[BUFFER_SIZE];
        strncpy(parse_buffer, buffer, sizeof(parse_buffer) - 1);
        parse_buffer[sizeof(parse_buffer) - 1] = '\0';

        Pipeline pipeline;
        if (parse_input(parse_buffer, &pipeline) && pipeline.num_commands == 1) {
            char *cmd = pipeline.commands[0].argv[0];

            if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "cd_new") == 0) {
                struct timespec cd_start, cd_end;
                clock_gettime(CLOCK_MONOTONIC, &cd_start);
                handle_cd_session(&pipeline.commands[0], &session);

                clock_gettime(CLOCK_MONOTONIC, &cd_end);
                long cd_runtime_ms = get_elapsed_ms(cd_start, cd_end);

                /* Direct session commands still send scheduler footer so client doesn't block. */
                char scheduler_info[256];
                snprintf(scheduler_info, sizeof(scheduler_info),
                         "\n[scheduler] job=0 wait_ms=0 runtime_ms=%ld quantum_ms=%d exit=0\n",
                         cd_runtime_ms, TIME_QUANTUM_MS);
                send_all(client_fd, scheduler_info, strlen(scheduler_info));
                send_end_marker(client_fd);
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
            send_end_marker(client_fd);
        }
    }

    log_printf("Client %d disconnected from %s:%d\n", ctx->session_id, ctx->ip, ctx->port);

    shell_session_set_current(NULL);
    close(client_fd);
}

void *client_thread(void *arg) {
    ClientContext *ctx = (ClientContext *)arg;
    if (ctx == NULL) {
        return NULL;
    }

    handle_client(ctx);
    free(ctx);

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
    log_printf("Server listening on port %d\n", PORT);
    log_printf("Dispatcher thread started for job scheduling\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);

        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t tid;
        ClientContext *ctx = malloc(sizeof(ClientContext));

        if (ctx == NULL) {
            perror("malloc failed");
            close(client_fd);
            continue;
        }

        pthread_mutex_lock(&session_id_mutex);
        ctx->session_id = next_session_id++;
        pthread_mutex_unlock(&session_id_mutex);

        ctx->client_fd = client_fd;
        ctx->port = ntohs(client_addr.sin_port);
        if (inet_ntop(AF_INET, &client_addr.sin_addr, ctx->ip, sizeof(ctx->ip)) == NULL) {
            strncpy(ctx->ip, "unknown", sizeof(ctx->ip) - 1);
            ctx->ip[sizeof(ctx->ip) - 1] = '\0';
        }

        if (pthread_create(&tid, NULL, client_thread, ctx) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(ctx);
            continue;
        }

        pthread_detach(tid);
    }

    close(server_fd);
    queue_destroy(global_queue);
    return 0;
}
