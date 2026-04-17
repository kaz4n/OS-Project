#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

/* Connect to the server and return the socket */
int connect_to_server(const char *ip, int port) {
    int sock;
    struct sockaddr_in serv_addr;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        return -1;
    }

    // Fill server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IP address from text to binary
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("invalid address");
        close(sock);
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        close(sock);
        return -1;
    }

    printf("Connected to server at %s:%d\n", ip, port);
    return sock;
}

/* Send commands to the server and print the response */
void client_loop() {
    char input[1024];
    char buffer[1024];
    int sock;
    int n;

    while (1) {
        printf("remote-shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (strcmp(input, "exit\n") == 0) {
            break;
        }

        sock = connect_to_server("127.0.0.1", PORT);
        if (sock < 0) {
            printf("Could not connect to server.\n");
            continue;
        }

        send(sock, input, strlen(input), 0);

        while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
        }

        close(sock);
    }
}



int main() {
    client_loop();
    return 0;
}