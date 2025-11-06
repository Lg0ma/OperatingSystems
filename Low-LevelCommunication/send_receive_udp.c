#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define STDIN_READ_SIZE 480                // Size of buffer for reading from standard input
#define UDP_RECV_BUFFER_SIZE 65536         // Size of buffer for receiving UDP data
#define EOF_SIGNAL "EOF_SHUTDOWN_SIGNAL"   // Signal to indicate end of file

// Function to handle writing data in chunks to avoid partial writes
ssize_t better_write(int fd, const char *buf, size_t count) {
    size_t already_written = 0;
    size_t to_be_written = count;
    ssize_t res_write;

    // Write data in chunks until all data is written
    while (to_be_written > 0) {
        size_t max_count = (to_be_written > 8192) ? 8192 : to_be_written;
        res_write = write(fd, &buf[already_written], max_count);

        if (res_write < 0) return res_write;   // Error handling
        if (res_write == 0) return already_written; // EOF handling

        already_written += res_write;
        to_be_written -= res_write;
    }
    return already_written;
}

// Function to count characters in a string until null terminator
size_t str_count(char* str) {
    size_t count = 0;
    size_t i = 0;

    while (str[i] != '\0') {
        count++;
        i++;
    }
    return count;
}

// Custom memory comparison function using basic system calls
int my_memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    // Loop through bytes to compare each byte
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] < p2[i]) ? -1 : 1;
        }
    }
    return 0;
}

// Custom function to check if a file descriptor is associated with a terminal
int my_isatty(int fd) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
        return -1;  // Error occurred
    }
    return S_ISCHR(st.st_mode);  // Check if character device
}

int main(int argc, char **argv) {
    char stdin_buffer[STDIN_READ_SIZE];    // Buffer for standard input data
    char udp_recv_buffer[UDP_RECV_BUFFER_SIZE];   // Buffer for UDP received data
    char *server_name;   // Server name from command line arguments
    char *port_name;     // Port name from command line arguments
    int sockfd = -1;
    struct addrinfo hints, *result, *curr;
    ssize_t bytes_read, bytes_received;
    int is_pipe = !my_isatty(STDIN_FILENO);  // Check if input is piped

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server> <port>\n", argv[0]);
        return -1;
    }

    server_name = argv[1];
    port_name = argv[2];

    // Set up network connection
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // Use IPv4
    hints.ai_socktype = SOCK_DGRAM;   // Datagram socket (UDP)

    // Resolve server address
    int gai_code = getaddrinfo(server_name, port_name, &hints, &result);
    if (gai_code != 0) {
        fprintf(stderr, "Could not resolve server address %s %s: %s\n", server_name, port_name, gai_strerror(gai_code));
        return -1;
    }

    // Loop through results to find a valid socket and connect
    for (curr = result; curr != NULL; curr = curr->ai_next) {
        sockfd = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (sockfd < 0) continue;

        if (connect(sockfd, curr->ai_addr, curr->ai_addrlen) == 0) break; // Successful connection

        close(sockfd);  // Close and try next if unsuccessful
        sockfd = -1;
    }

    if (sockfd < 0) {
        fprintf(stderr, "Could not create/connect socket: %s\n", strerror(errno));
        freeaddrinfo(result);
        return -1;
    }
    freeaddrinfo(result);

    if (is_pipe) {
        // Handle piped input data
        while ((bytes_read = read(STDIN_FILENO, stdin_buffer, STDIN_READ_SIZE)) > 0) {
            if (send(sockfd, stdin_buffer, bytes_read, 0) < 0) {
                fprintf(stderr, "Error sending data: %s\n", strerror(errno));
                break;
            }

            bytes_received = recv(sockfd, udp_recv_buffer, UDP_RECV_BUFFER_SIZE, 0);
            if (bytes_received < 0) {
                fprintf(stderr, "Error receiving data: %s\n", strerror(errno));
                break;
            }
            if (bytes_received == 0) break;

            if (better_write(STDOUT_FILENO, udp_recv_buffer, bytes_received) < 0) {
                fprintf(stderr, "Error writing to output: %s\n", strerror(errno));
                break;
            }
        }

        if (bytes_read == 0) {
            // Send EOF signal if end of input is reached
            if (send(sockfd, EOF_SIGNAL, str_count(EOF_SIGNAL), 0) < 0) {
                fprintf(stderr, "Error sending EOF signal: %s\n", strerror(errno));
            }
        }
    } else {
        // Interactive mode with select() for non-blocking I/O
        fd_set read_fds;
        while (1) {
            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);
            FD_SET(sockfd, &read_fds);

            if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0) {
                fprintf(stderr, "Select error: %s\n", strerror(errno));
                break;
            }

            // Check if there's input from stdin
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                bytes_read = read(STDIN_FILENO, stdin_buffer, STDIN_READ_SIZE);
                if (bytes_read <= 0) {
                    if (bytes_read == 0) {
                        // Send EOF signal at end of input
                        if (send(sockfd, EOF_SIGNAL, str_count(EOF_SIGNAL), 0) < 0) {
                            fprintf(stderr, "Error sending EOF signal: %s\n", strerror(errno));
                        }
                    }
                    break;
                }

                if (send(sockfd, stdin_buffer, bytes_read, 0) < 0) {
                    fprintf(stderr, "Error sending: %s\n", strerror(errno));
                    break;
                }
            }

            // Check if there's data received from the socket
            if (FD_ISSET(sockfd, &read_fds)) {
                bytes_received = recv(sockfd, udp_recv_buffer, UDP_RECV_BUFFER_SIZE, 0);
                if (bytes_received < 0) {
                    fprintf(stderr, "Receive error: %s\n", strerror(errno));
                    break;
                }

                // Handle received EOF signal
                if (bytes_received == strlen(EOF_SIGNAL) && 
                    my_memcmp(udp_recv_buffer, EOF_SIGNAL, str_count(EOF_SIGNAL)) == 0) {
                    printf("Received shutdown signal, terminating...\n");
                    break;
                }

                if (better_write(STDOUT_FILENO, udp_recv_buffer, bytes_received) < 0) {
                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                    break;
                }
            }
        }
    }

    close(sockfd);  // Close socket before exit
    return 0;
}
