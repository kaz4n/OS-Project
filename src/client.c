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
#define END_OF_OUTPUT_MARKER "\n<<END_OF_OUTPUT>>\n"

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

/* Receive server response until end marker appears. */
int receive_response_until_marker(int sock, const char *marker) {
    char buffer[BUFFER_SIZE];
    char *accum = NULL;
    size_t accum_len = 0;
    size_t marker_len = strlen(marker);

    while (1) {
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (n > 0) {
            buffer[n] = '\0';

            char *new_accum = realloc(accum, accum_len + (size_t)n + 1);
            if (new_accum == NULL) {
                free(accum);
                fprintf(stderr, "client: memory error\n");
                return 0;
            }

            accum = new_accum;
            memcpy(accum + accum_len, buffer, (size_t)n + 1);
            accum_len += (size_t)n;

            char *marker_pos = strstr(accum, marker);
            if (marker_pos != NULL) {
                size_t printable = (size_t)(marker_pos - accum);
                if (printable > 0) {
                    fwrite(accum, 1, printable, stdout);
                    fflush(stdout);
                }
                free(accum);
                return 1;
            }

            /* Stream everything except a small suffix that may contain split marker bytes. */
            if (accum_len > marker_len) {
                size_t keep = marker_len - 1;
                size_t flush_len = accum_len - keep;
                fwrite(accum, 1, flush_len, stdout);
                fflush(stdout);
                memmove(accum, accum + flush_len, keep);
                accum[keep] = '\0';
                accum_len = keep;
            }
        } else if (n == 0) {
            if (accum_len > 0) {
                fwrite(accum, 1, accum_len, stdout);
                fflush(stdout);
            }
            free(accum);
            return 0;
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            perror("recv failed");
            free(accum);
            return 0;
        }
    }
}

/* Send commands to the server and print the response */
void client_loop(const char *server_ip) {
    char input[1024];
    int sock;

    /* Establish a single persistent connection */
    sock = connect_to_server(server_ip, PORT);
    if (sock < 0) {
        printf("Failed to connect to server.\n");
        return;
    }

    /* Read and print welcome banner from server. */
    receive_response_until_marker(sock, END_OF_OUTPUT_MARKER);

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

        /* Receive command output until explicit protocol marker arrives. */
        if (!receive_response_until_marker(sock, END_OF_OUTPUT_MARKER)) {
            break;
        }
    }

    close(sock);
}



int main(int argc, char **argv) {
    const char *server_ip = SERVER_IP;

    if (argc > 1 && argv[1] != NULL && argv[1][0] != '\0') {
        server_ip = argv[1];
    }

    client_loop(server_ip);
    return 0;
}