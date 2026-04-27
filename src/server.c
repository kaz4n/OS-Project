#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "shell.h"
#include <pthread.h>
#include "scheduler.h"


#define PORT 8080
#define BUFFER_SIZE 1024

static Scheduler g_scheduler;

void handle_client(int client_fd);

static int send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) {
            return 0;
        }
        sent += (size_t)n;
    }

    return 1;
}

void *client_thread(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    handle_client(client_fd);

    return NULL;
}

int setup_server_socket(int port) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

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
    char buffer[1024];
    int n;
    Job *job;

    memset(buffer, 0, sizeof(buffer));

    n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return;
    }

    buffer[n] = '\0';
    buffer[strcspn(buffer, "\r\n")] = '\0';

    job = scheduler_submit(&g_scheduler, client_fd, buffer);
    if (job == NULL) {
        const char *msg = "scheduler: failed to queue job\n";
        send_all(client_fd, msg, strlen(msg));
        close(client_fd);
        return;
    }

    scheduler_wait(job);

    if (job->output != NULL && job->output_len > 0) {
        send_all(client_fd, job->output, job->output_len);
    }

    scheduler_destroy_job(job);

    close(client_fd);
}

int main() {
    if (!scheduler_init(&g_scheduler, DEFAULT_QUANTUM_MS)) {
        fprintf(stderr, "Failed to initialize scheduler\n");
        return 1;
    }

    int server_fd = setup_server_socket(PORT);

 while (1) {
        int client_fd = accept_client(server_fd);

        if (client_fd < 0) {
            continue;
        }

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
    scheduler_shutdown(&g_scheduler);
    return 0;
}