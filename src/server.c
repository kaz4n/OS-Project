#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "shell.h"

#define PORT 8080
#define BUFFER_SIZE 1024

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

    if (listen(server_fd, 1) < 0) {
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

void execute_remote_command(int client_fd, char *command) {
    int saved_stdout;
    char buff[BUFFER_SIZE];

    strncpy(buff, command, sizeof(buff) - 1);
    buff[sizeof(buff) - 1] = '\0';

    saved_stdout = dup(STDOUT_FILENO);
    dup2(client_fd, STDOUT_FILENO);

    process_input(buff);

    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
            break;

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        if (strcmp(buffer, "exit") == 0)
            break;

        execute_remote_command(client_fd, buffer);
    }

    close(client_fd);
}

int main() {
    int server_fd = setup_server_socket(PORT);

    while (1) {
        int client_fd = accept_client(server_fd);
        if (client_fd < 0)
            continue;

        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}