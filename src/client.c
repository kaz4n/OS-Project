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
void client_loop(int sock) {
    char input[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (1) {
        printf("remote-shell> ");

        // Read command from user
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Send command to server
        send(sock, input, strlen(input), 0);

        // If user typed exit, stop the client too
        if (strcmp(input, "exit\n") == 0) {
            break;
        }

        // Clear buffer before receiving
        memset(buffer, 0, sizeof(buffer));

        // Receive response from server
        bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        buffer[bytes_received] = '\0';

        // Print server response
        printf("%s", buffer);
    }
}

/* Main client program */
int main() {
    int sock;

    sock = connect_to_server(SERVER_IP, PORT);
    if (sock < 0) {
        return 1;
    }

    client_loop(sock);

    close(sock);
    return 0;
}