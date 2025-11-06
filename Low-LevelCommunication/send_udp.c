#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>

#define BUFFER_LEN 480

size_t str_count(char* str) {
  size_t count, i;

  count = (size_t) 0;
  i = (size_t) 0;

  /* Count str character pointer by
     finding null terminator  */
  while (str[i] != '\0') {
    count ++;
    i ++;
  }

  return count;
}

void my_usleep() {
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = 50000;

    if (nanosleep(&req, NULL) < 0) {
        perror("nanosleep failed");
    }
}


int main(int argc, char **argv) {
    char buffer[BUFFER_LEN];
    char *server_name, *port_name;
    int sockfd;
    struct addrinfo hints, *result, *curr;

    // Ensure the program has the required arguments
    if (argc < 3) { 
        fprintf(stderr, "Usage: %s <server> <port>\n", argv[0]);
        return 1;
    }
    server_name = argv[1];
    port_name = argv[2];

    // Set up hints for UDP socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP Socket
    hints.ai_protocol = 0;           // Any Protocol

    // Get address info
    int gai_code = getaddrinfo(server_name, port_name, &hints, &result);
    if (gai_code != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(gai_code));
        return -1;
    }

    // Iterate through the address info results to create and connect the socket
    for (curr = result; curr != NULL; curr = curr->ai_next) {
        sockfd = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (sockfd < 0) {
          fprintf(stderr, "Error creating socked: %s\n", strerror(errno));
          return -1;;
        }

        if (connect(sockfd, curr->ai_addr, curr->ai_addrlen) == 0) {
            break;  // Successfully connected
        } else {
            close(sockfd);  // Close and try the next address if connect fails
        }
    }

    // Check if we successfully connected
    if (curr == NULL) {
        fprintf(stderr, "Could not create or connect socket to %s on port %s\n", server_name, port_name);
        freeaddrinfo(result);
        return -1;
    }

    ssize_t bytes_read;
    // Read and send each chunk of data
    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_LEN)) > 0) {
        // Send the data read from standard input as UDP packet
        if (send(sockfd, buffer, bytes_read, 0) < 0) {
            fprintf(stderr, "send error 1: %s\n", strerror(errno));
            close(sockfd);
            freeaddrinfo(result);
            return 1;
        }
    }

    if (bytes_read < 0) {
      fprintf(stderr, "read error: %s\n", strerror(errno));
      close(sockfd);
      freeaddrinfo(result);
      return -1;
    }

    // EOF detected
    char *eof_message = "EOF";
    int send_attempts = 20;
    int attempts = 0;

    while (attempts < send_attempts) {
      if (send(sockfd, eof_message, str_count(eof_message), 0) < 0) {
        break;
      }
      attempts++;
      my_usleep();
    }

    // Clean up: close socket and free address info
    close(sockfd);
    freeaddrinfo(result);

    return 0;
}