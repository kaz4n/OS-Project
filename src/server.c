#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include "shell.h"

#define PORT 8080
#define BUFFER_SIZE 1024

static int send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

static int execute_and_send(int client_fd, const char *command) {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) != 0) {
        perror("pipe");
        return 0;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]); close(pipefd[1]);
        return 0;
    }

    if (pid == 0) {
        /* child: redirect stdout/stderr to pipe and run command */
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(1);
        if (dup2(pipefd[1], STDERR_FILENO) < 0) _exit(1);
        close(pipefd[1]);
        process_input((char *)command);
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

    return 1;
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

    memset(buffer, 0, sizeof(buffer));

    n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return;
    }

    buffer[n] = '\0';
    buffer[strcspn(buffer, "\r\n")] = '\0';

    if (!execute_and_send(client_fd, buffer)) {
        const char *err = "server: failed to execute command\n";
        send_all(client_fd, err, strlen(err));
    }

    close(client_fd);
}

void *client_thread(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    handle_client(client_fd);

    return NULL;
}

int main() {
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
    return 0;
}
