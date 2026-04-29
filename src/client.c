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

    /* Set a short receive timeout to keep the client responsive when server closes connection unexpectedly.
       We will still wait for the scheduler footer after each command. */
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

        /* Receive response from server until scheduler footer is seen */
        char *accum = NULL;
        size_t accum_len = 0;
        int footer_found = 0;

        while (!footer_found) {
            memset(buffer, 0, sizeof(buffer));
            n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                /* print chunk immediately */
                printf("%s", buffer);

                /* append to accumulator for footer search */
                char *new_accum = realloc(accum, accum_len + n + 1);
                if (new_accum == NULL) {
                    free(accum);
                    fprintf(stderr, "client: memory error\n");
                    break;
                }
                accum = new_accum;
                memcpy(accum + accum_len, buffer, n + 1);
                accum_len += n;

                if (strstr(accum, "[scheduler]") != NULL) {
                    footer_found = 1;
                }
            } else if (n == 0) {
                /* server closed connection */
                break;
            } else {
                /* n < 0: check for timeout and continue waiting for footer */
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    /* timeout — continue waiting for footer */
                    continue;
                } else {
                    perror("recv failed");
                    break;
                }
            }
        }

        free(accum);
    }

    close(sock);
}



int main() {
    client_loop();
    return 0;
}