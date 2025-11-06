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
#define RECON_BUFFER_SIZE 131076
#define EOF_SIGNAL "EOF_SHUTDOWN_SIGNAL"


static int convert_port_name(uint16_t *port, const char *port_name) {
    char *end;
    long long int nn;
    uint16_t t;
    long long int tt;

    if (port_name == NULL || *port_name == '\0')
        return -1;

    nn = strtoll(port_name, &end, 0);
    if (*end != '\0' || nn < 0)
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
  count = (size_t) 0;
  i = (size_t) 0;

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
    size_t already_written = 0, to_be_written = count;
    ssize_t res_write;

    if (count == 0)
        return 0;

    while (to_be_written > 0) {
        size_t max_count = (to_be_written > 8192) ? 8192 : to_be_written;
        res_write = write(fd, &buf[already_written], max_count);

        if (res_write < 0)
            return res_write;
        if (res_write == 0)
            return already_written;

        already_written += res_write;
        to_be_written -= res_write;
    }
    return already_written;
}

int main(int argc, char **argv) {
    uint16_t tcp_port;
    char *udp_server, *udp_port;
    int tcp_listening_sock, tcp_conn_sock, udp_sock, gai_code;
    fd_set read_fds;
    char tcp_buff[BUFFER_SIZE + 2], udp_buff[BUFFER_SIZE + 2];
    ssize_t tcp_recv = 0;
    struct addrinfo hints, *result = NULL, *curr;
    struct sockaddr_in tcp_addr;


    // Check arguments
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <TCP port> <UDP server> <UDP port>\n", argv[0]);
        return -1;
    }

    // Convert TCP port
    if (convert_port_name(&tcp_port, argv[1]) < 0) {
        fprintf(stderr, "Invalid TCP port: %s\n", argv[1]);
        return -1;
    }
    udp_server = argv[2];
    udp_port = argv[3];

    // Create tcp_listening_sock
    tcp_listening_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_listening_sock < 0) {
      fprintf(stderr, "Error creating listening sock: %s\n", strerror(errno));
      return -1;
    }

    // Set up TCP
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(tcp_port);

    // Bind TCP sock
    if (bind(tcp_listening_sock, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
      fprintf(stderr, "Failed to bind TCP socket: %s\n", strerror(errno)); 
       if(close(tcp_listening_sock) < 0) {
   fprintf(stderr, "Error closing tcp_listen_sock: %s\n", strerror(errno)); 
       } 
       return -1; 
    }

    if (listen(tcp_listening_sock, 1) < 0) {
        fprintf(stderr, "Failed to listen on TCP socket: %s\n", strerror(errno));
        if(close(tcp_listening_sock) < 0) {
            fprintf(stderr, "Error closing tcp_listen_sock: %s\n", strerror(errno));
        }
        return -1;
    }

    // SET up UDP
    memset(&hints, 0, sizeof(hints)); 
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_DGRAM; 
    hints.ai_protocol = 0;

    gai_code = getaddrinfo(udp_server, udp_port, &hints, &result); 
    if (gai_code != 0) {
      fprintf(stderr, "Could not get UDP address: %s\n", strerror(errno)); 
      if(close(tcp_listening_sock) < 0) {
  fprintf(stderr, "Error closing tcp_listen_sock: %s\n", strerror(errno));
      }
      return -1;
    }

    // Create UDP sock and try to connect to server
    for (curr = result; curr != NULL; curr = curr->ai_next) {
      udp_sock = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
      if (udp_sock < 0) {
  continue;
      }
      if (connect(udp_sock, curr->ai_addr, curr->ai_addrlen) == 0) {
  break;
      }

      if(close(udp_sock) < 0) {
  if(close(udp_sock) < 0) {
    fprintf(stderr, "Error closing tcp_listen_sock: %s\n", strerror(errno));
  }
      }
    }

    if (curr == NULL) {
      fprintf(stderr, "Failed to create/connect UDP sock: %s\n", strerror(errno));
      if(close(tcp_listening_sock) < 0) {
        fprintf(stderr, "Error closing tcp_listening_sock: %s\n", strerror(errno));
      }
      freeaddrinfo(result);
      return -1;
    }

    char *waitingMessage = "Waiting for TCP connection... \n";
    better_write(1, waitingMessage, str_count(waitingMessage));

    tcp_conn_sock = accept(tcp_listening_sock, NULL, NULL);
    if (tcp_conn_sock < 0) {
      fprintf(stderr, "Failed to accept a TCP connection: %s\n", strerror(errno));
      if(close(tcp_listening_sock) < 0) {
        fprintf(stderr, "Error closing tcp_listen_sock: %s\n", strerror(errno));
      }
      if(close(udp_sock) < 0) {
        fprintf(stderr, "Error closing tcp_listen_sock: %s\n", strerror(errno));
      }
      freeaddrinfo(result);
      return -1;
    }

    char *acceptanceMessage = "TCP connection established. Starting Tunnel.\n";
    better_write(1, acceptanceMessage, str_count(acceptanceMessage));
  // Listen for packets on either tcp or udp
    while(1) {
      FD_ZERO(&read_fds);
      FD_SET(udp_sock, &read_fds);
      FD_SET(tcp_conn_sock, &read_fds);

      int fd = (udp_sock > tcp_conn_sock ? udp_sock : tcp_conn_sock) + 1;

      if (select(fd, &read_fds, NULL, NULL, NULL) < 0) {
  fprintf(stderr, "Select failed: %s\n", strerror(errno));
  break;
      }
  //
      if (FD_ISSET(udp_sock, &read_fds)) {
  ssize_t udp_packet_size = recv(udp_sock, udp_buff + 2, BUFFER_SIZE - 2, 0);
  if (udp_packet_size < 0) {
    fprintf(stderr, "Failed to received UDP packet: %s\n", strerror(errno));
    continue;
  }

  *((uint16_t *)udp_buff) = htons((uint16_t) udp_packet_size);

  if (better_write(tcp_conn_sock , udp_buff, udp_packet_size + 2) < 0) {
    fprintf(stderr, "Failed to send message over TCP: %s\n", strerror(errno));
    break;
  }
      }

      if (FD_ISSET(tcp_conn_sock, &read_fds)) {
  ssize_t tcp_bytes_read = read(tcp_conn_sock, tcp_buff + tcp_recv, BUFFER_SIZE - tcp_recv);

   if (tcp_bytes_read <= 0) {
        if (tcp_bytes_read == 0) {
            // Send EOF signal to UDP server before shutting down
            const char* eof_signal = EOF_SIGNAL;
            if (send(udp_sock, eof_signal, strlen(eof_signal), 0) < 0) {
                fprintf(stderr, "Failed to send EOF to UDP server: %s\n", strerror(errno));
            }
            char *warningMessage = "TCP connection closed, shutting down\n";
            better_write(1, warningMessage, str_count(warningMessage));
        } else {
            fprintf(stderr, "TCP read error occurred: %s\n", strerror(errno));
        }
        break;
    }

  tcp_recv += tcp_bytes_read;

  // Process finished messages
  while (tcp_recv > 2) {
    if(sizeof(udp_buff) == 2 ){
      break;
    }
    uint16_t packet_len = ntohs(*((uint16_t *)tcp_buff));
    if (tcp_recv >= (packet_len + 2)) {
      // Send the message UDP
      if (send(udp_sock, tcp_buff + 2, packet_len, 0) < 0) {
        fprintf(stderr, "Failed to send message over UDP: %s\n", strerror(errno));
        goto cleanup;
      }

      // Move remaining data to beggining of buffer
      memmove(tcp_buff, tcp_buff + packet_len + 2, tcp_recv - packet_len - 2);
      tcp_recv -= packet_len + 2;
    }
    else if(tcp_recv == 2){
      break;
    }
  }
      }
    }

 cleanup:
    // Clean all remaining info
    if (close(tcp_conn_sock) < 0) {
        fprintf(stderr, "Error closing TCP connection: %s\n", strerror(errno));
  return -1;
    }
    if (close(tcp_listening_sock) < 0) {
        fprintf(stderr, "Error closing TCP listen socket: %s\n", strerror(errno));
  return -1;
    }
    if (close(udp_sock) < 0) {
        fprintf(stderr, "Error closing UDP socket: %s\n", strerror(errno));
  return -1;
    }
    freeaddrinfo(result);
    return 0;
}
