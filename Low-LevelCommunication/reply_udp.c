#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#define BUFFER_SIZE 65536
#define EOF_SIGNAL "EOF_SHUTDOWN_SIGNAL"


// Simplified memcmp to my_memcmp using level 2 system calls                                                                                                                              
int my_memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;

  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return (p1[i] < p2[i]) ? -1 : 1;
    }
  }
  return 0;
}

static int convert_port_name(uint16_t *port, const char *port_name) {
    char *end;
    long long int nn;
    uint16_t t;
    long long int tt;
    if (port_name == NULL) return -1;
    if (*port_name == '\0') return -1;
    nn = strtoll(port_name, &end, 0);
    if (*end != '\0') return -1;
    if (nn < ((long long int) 0)) return -1;
    t = (uint16_t) nn;
    tt = (long long int) t;
    if (tt != nn) return -1;
    *port = t;
    return 0;
}

ssize_t better_write(int fd, const char *buf, size_t count) {
  size_t already_written, to_be_written, written_this_time, max_count;
  ssize_t res_write;

  if (count == ((size_t) 0)) return (ssize_t) count;

  already_written = (size_t) 0;
  to_be_written = count;
  while (to_be_written > ((size_t) 0)) {
    max_count = to_be_written;
    if (max_count > ((size_t) 8192)) {
      max_count = (size_t) 8192;
    }
    res_write = write(fd, &(((const char *) buf)[already_written]), max_count);
    if (res_write <  (size_t) 0) {
      /* Error */
      return res_write;
    }
    if (res_write == ((ssize_t) 0)) {
      /* Nothing written, stop trying */
      return (ssize_t) already_written;
    }
    written_this_time = (size_t) res_write;
    already_written += written_this_time;
    to_be_written -= written_this_time;
  }
  return (ssize_t) already_written;
}

int main(int argc, char **argv) {
    int sockfd;
    char buffer[BUFFER_SIZE + 32];
    // Need to store the "Received: " + actual message
    struct sockaddr_in servaddr, clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);
    uint16_t port;
    ssize_t bytes_received;

    // Check if correct number of arguments were passed
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments, usage: %s <port>\n", argv[0]);
        return -1;
    }

    // Convert port name to 16-bit integer
    if (convert_port_name(&port, argv[1]) < 0) {
        fprintf(stderr, "Invalid Port Number: %s\n", argv[1]);
        return -1;
    }

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
        return -1;
    }

    // Set up the server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;          // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;  // Bind to any address
    servaddr.sin_port = htons(port);        // Port number

    // Bind the socket to the port
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "Bind failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    // Listening message
    fprintf(stdout, "Listening on port %u...\n", port);

    // Continuously receive data from clients
    while (1) {
      // Reset clientaddr_len before each recvfrom
      clientaddr_len = sizeof(clientaddr);

      bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, &clientaddr_len);
      if (bytes_received < 0) {
  fprintf(stderr, "recvfrom failed: %s\n", strerror(errno));
  continue;
      }

    if (bytes_received == strlen(EOF_SIGNAL) && 
        my_memcmp(buffer, EOF_SIGNAL, strlen(EOF_SIGNAL)) == 0) {
        printf("Received shutdown signal, terminating...\n");
        break;  // Exit the loop and terminate
    }

      // Null-terminate the buffer if receiving text data
      if (bytes_received < BUFFER_SIZE) {
  buffer[bytes_received] = '\0';
      }

      // Print to stdout the received message
      //better_write(1, "Received: ", 10);
      // better_write(1, buffer, bytes_received + 1);


      if(clientaddr.sin_port == 0 || clientaddr.sin_addr.s_addr == INADDR_NONE) {
  fprintf(stderr, "Invalid client address received, cannot reply back\n");
  return -1;
      }

      // Reply to client after receiving message
      ssize_t bytes_sent = sendto(sockfd, buffer, bytes_received, 0, (struct sockaddr *)&clientaddr, clientaddr_len);
      if (bytes_sent < 0) {
  fprintf(stderr, "Error replying to client: %s\n", strerror(errno));
  return -1;
      }
    }


    // Close the socket
    close(sockfd);
    return 0;
}
