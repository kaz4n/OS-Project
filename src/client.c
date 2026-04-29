#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define RECV_TIMEOUT_MS 100  /* 100ms timeout for recv() to handle persistent connections */

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

    /* Set receive timeout to allow responsive multi-command handling */
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RECV_TIMEOUT_MS * 1000;  /* Convert ms to microseconds */
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO failed");
        close(sock);
        return -1;
    }

    printf("Connected to server at %s:%d\n", ip, port);
    return sock;
}

/* Send commands to the server and print the response */
void client_loop() {
    char input[1024];
    char buffer[BUFFER_SIZE];
    int sock;
    int n;

    /* Establish a single persistent connection */
    sock = connect_to_server("127.0.0.1", PORT);
    if (sock < 0) {
        printf("Failed to connect to server.\n");
        return;
    }

    while (1) {
        printf("remote-shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        /* Send command to server */
        if (send(sock, input, strlen(input), 0) < 0) {
            perror("send failed");
            break;
        }

        /* Check for exit command */
        if (strncmp(input, "exit", 4) == 0) {
            printf("Disconnecting from server.\n");
            break;
        }

        /* Receive response from server */
        memset(buffer, 0, sizeof(buffer));
        while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
            memset(buffer, 0, sizeof(buffer));
        }
        /* recv() timeout or end of response is normal; proceed to next command */
    }

    close(sock);
}



int main() {
    client_loop();
    return 0;
}