#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 65536
#define BUFFER_LEN ((size_t)4096)
#define EOF_SIGNAL "EOF_SHUTDOWN_SIGNAL"


static int convert_port_name(uint16_t *port, const char *port_name) {
  char *end;
  long long int nn;
  uint16_t t;
  long long int tt;
  if (port_name == NULL)
    return -1;
  if (*port_name == '\0')
    return -1;
  nn = strtoll(port_name, &end, 0);
  if (*end != '\0')
    return -1;
  if (nn < ((long long int)0))
    return -1;
  t = (uint16_t)nn;
  tt = (long long int)t;
  if (tt != nn)
    return -1;
  *port = t;
  return 0;
}

size_t str_count(char *str) {
  size_t count, i;
  count = (size_t)0;
  i = (size_t)0;

  /* Count str character pointer by
     finding null terminator
  */
  while (str[i] != '\0') {
    count++;
    i++;
  }

  return count;
}

ssize_t better_write(int fd, const char *buf, size_t count) {
  size_t already_written, to_be_written, written_this_time, max_count;
  ssize_t res_write;

  if (count == ((size_t)0))
    return (ssize_t)count;

  already_written = (size_t)0;
  to_be_written = count;
  while (to_be_written > ((size_t)0)) {
    max_count = to_be_written;
    if (max_count > ((size_t)8192)) {
      max_count = (size_t)8192;
    }
    res_write = write(fd, &(((const char *)buf)[already_written]), max_count);
    if (res_write < ((size_t)0)) {
      /* Error */
      return res_write;
    }
    if (res_write == ((ssize_t)0)) {
      /* Nothing written, stop trying */
      return (ssize_t)already_written;
    }
    written_this_time = (size_t)res_write;
    already_written += written_this_time;
    to_be_written -= written_this_time;
  }
  return (ssize_t)already_written;
}

ssize_t write_string(int fd, char *str) {
  size_t size;

  size = strlen(str);
  return better_write(fd, str, size);
}

int run_client(int fd, char *str) {
  char buffer[BUFFER_LEN];
  ssize_t read_res;
  size_t bytes_to_write;

  /* Write the string str to the server */
  if (write_string(fd, str) < ((ssize_t)0)) {
    fprintf(stderr, "Cannot write: %s\n", strerror(errno));
    return -1;
  }

  /* Read the converted string from the server */
  read_res = read(fd, buffer, sizeof(buffer));
  if (read_res < ((ssize_t)0)) {
    fprintf(stderr, "Cannot read: %s\n", strerror(errno));
    return -1;
  }

  /* Write out the converted string to standard output */
  bytes_to_write = (size_t)read_res;
  if (better_write(1, buffer, bytes_to_write) < ((ssize_t)0)) {
    fprintf(stderr, "Cannot write: %s\n", strerror(errno));
    return -1;
  }

  /* Print a newline to standard output */
  if (write_string(1, "\n") < ((ssize_t)0)) {
    fprintf(stderr, "Cannot write: %s\n", strerror(errno));
    return -1;
  }

  /* Return success */
  return 0;
}



int main(int argc, char **argv) {
  uint16_t udp_port;
  char *server_name, *port_name;
  int udp_sock, tcp_sock, gai_code; //found;
  fd_set read_fds;
  char udp_buffer[BUFFER_SIZE + 2], tcp_buffer[BUFFER_SIZE + 2];
  ssize_t tcp_received = 0;

  struct sockaddr_in udp_addr, udp_client_addr;
  struct addrinfo *result, *curr, hints;
  socklen_t client_len;

  // Check arguments
  if (argc < 4) {
    fprintf(stderr, "Not enough arguments, usage: %s <UDP port> <TCP srvr> <TCP port>\n", argv[0]);
    return -1;
  }
  // TCP info
  server_name = argv[2];
  port_name = argv[3];

  // Convert UDP port
  if (convert_port_name(&udp_port, argv[1]) < 0) {
    fprintf(stderr, "UDP Port Number is invalid: %s\n", argv[1]);
    return -1;
  }

  memset(&udp_addr, 0, sizeof(udp_addr));
  udp_addr.sin_family = AF_INET;
  udp_addr.sin_addr.s_addr = INADDR_ANY;
  udp_addr.sin_port = htons(udp_port);

  // Create UDP port socket & set up connection
  udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_sock < 0) {
    fprintf(stderr, "Failed to create UDP socket: %s\n", strerror(errno));
    return -1;
  }


  if (bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
    fprintf(stderr, "Failed to bind UDP sock: %s\n", strerror(errno));
    if (close(udp_sock) < 0) {
      fprintf(stderr, "Failed to close sock %s\n", strerror(errno));
    }
    return -1;
  }

  // Set up TCP connection
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = 0;

  gai_code = getaddrinfo(server_name, port_name, &hints, &result);
  if (gai_code != 0) {
    fprintf(stderr, "Failed to get TCP server address: [%s, %s]\n", server_name,
            port_name);
    close(close(udp_sock));
    return -1;
  }

  // Iterate over linked list return by addrinfo
  for (curr = result; curr != NULL; curr = curr->ai_next) {
    tcp_sock = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
    if (tcp_sock < 0) {
      fprintf(stderr, "Couldn't create socket: %s\n", strerror(errno));
      freeaddrinfo(result);
      return -1;
    }
    if (connect(tcp_sock, curr->ai_addr, curr->ai_addrlen) == 0) {
      break;
    }
    if (close(tcp_sock) < 0) {
      fprintf(stderr, "Failed to close tcp_sock %s\n", strerror(errno));
      freeaddrinfo(result);
      return 1;
    }
    tcp_sock = -1;
  }

  char * tunnel_message = "Tunnel UDP over TCP running\n";
  better_write(1, tunnel_message, str_count(tunnel_message));
  int closeConnection = 0;

  while(1 && closeConnection == 0) {
    FD_ZERO(&read_fds);
    FD_SET(udp_sock, &read_fds);
    FD_SET(tcp_sock, &read_fds);

    size_t activity = (udp_sock > tcp_sock ? udp_sock : tcp_sock) + 1;

    if(select(activity, &read_fds, NULL, NULL, NULL) < 0) {
      fprintf(stderr, "Select failed: %s", strerror(errno));
      return -1;
    }
    client_len = sizeof(udp_client_addr); 

    // Need to handle UDP packets
    if (FD_ISSET(udp_sock, &read_fds)) {
      ssize_t packet_size = recvfrom(udp_sock, udp_buffer + 2, BUFFER_SIZE - 2, 0, (struct sockaddr *) &udp_client_addr, &client_len);

      if(packet_size < 0) {
        fprintf(stderr, "Failed to receive UDP packet: %s", strerror(errno));
        continue;
      }
      // Check for EOF signal
    if (packet_size == strlen(EOF_SIGNAL) && 
        memcmp(udp_buffer + 2, EOF_SIGNAL, strlen(EOF_SIGNAL)) == 0) {
        // Send EOF signal through TCP tunnel
        const char eof_packet[2] = {0, 0};  // Zero-length packet signals EOF
        if(better_write(tcp_sock, eof_packet, 2) < 0) {
            fprintf(stderr, "Failed to send EOF signal: %s", strerror(errno));
        }
        closeConnection = 1;
        break;
    }

      // Need to make UDP packet usable for byte transfer
      *((uint16_t *) udp_buffer)  = htons((uint16_t)packet_size);

      //Use TCP to send UDP packet with prefix length
      if(better_write(tcp_sock, udp_buffer, packet_size + 2) < 0) {
        fprintf(stderr, "Failed to send packet: %s", strerror(errno));
  return -1;
      }
    }

    // Handle Input from TCP
    if (FD_ISSET(tcp_sock, &read_fds)) {
      ssize_t tcp_bytes_received = read(tcp_sock, tcp_buffer + tcp_received, BUFFER_SIZE - tcp_received);
      if (tcp_bytes_received <= 0) {
        if (tcp_bytes_received == 0) {
            if (send(udp_sock, "", 0, 0) < 0) {
              fprintf(stderr, "Error sending EOF signal: %s\n", strerror(errno));
              return -1;
            }
          char *endMessage = "TCP connection closed by server\n";
          better_write(1, endMessage, str_count(endMessage));
    closeConnection = 1;
    break;
  }
        else {
          fprintf(stderr, "Failed to receive TCP data: %s", strerror(errno));
        }
        continue;
      }

      tcp_received += tcp_bytes_received;

      if (tcp_received >= 2) {
        uint16_t udp_packet_len = ntohs(*((uint16_t *) tcp_buffer));
        if (tcp_received >= (udp_packet_len + 2)) {
          // Need to send message using UDP, without 2-bye length
          if (sendto(udp_sock, tcp_buffer + 2, udp_packet_len, 0, (struct sockaddr *) &udp_client_addr, client_len) < 0) {
            fprintf(stderr, "Error sending message over UDP: %s" ,strerror(errno));
            break;
          }

          // Need to prepare buffer for next message
          memmove(tcp_buffer, tcp_buffer + udp_packet_len + 2, tcp_received - udp_packet_len - 2);
            tcp_received -= udp_packet_len + 2;
        }
      }
    }
  }

  // Clean up
  if(close(udp_sock) < 0) {
    fprintf(stderr, "Error closing the socket: %s", strerror(errno));
    return -1;
  }
  if(close(tcp_sock) < 0) {
    fprintf(stderr, "Error closing the socket: %s", strerror(errno));
    return -1;
  }
  freeaddrinfo(result);
  // Everthing went well, signal success
  return 0;
}