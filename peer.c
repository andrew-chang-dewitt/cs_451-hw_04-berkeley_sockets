/* enable POSIX.1-2001 for getaddrinfo, struct addrinfo, etc. */
#define _POSIX_C_SOURCE 200112L

#include <arpa/inet.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef PEERH
#include "peer.h"
#endif

Peer *peer_connect(const char *host, unsigned short port) {
  struct addrinfo hints;
  struct addrinfo *res;
  char port_str[6];
  int fd;
  Peer *peer;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  snprintf(port_str, sizeof(port_str), "%u", port);
  if (getaddrinfo(host, port_str, &hints, &res) != 0) {
    return NULL;
  }

  fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    freeaddrinfo(res);
    return NULL;
  }

  if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    freeaddrinfo(res);
    close(fd);
    return NULL;
  }

  freeaddrinfo(res);

  peer = malloc(sizeof(*peer));
  if (peer == NULL) {
    close(fd);
    return NULL;
  }

  peer->fd = fd;
  return peer;
}

Peer *peer_accept(unsigned short port) {
  int listen_fd;
  int conn_fd;
  int opt = 1;
  /* union avoids strict-aliasing violation when casting to struct sockaddr * */
  union {
    struct sockaddr_in in;
    struct sockaddr sa;
  } addr;
  socklen_t addr_len = sizeof(addr);
  Peer *peer;

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    return NULL;
  }

  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(listen_fd);
    return NULL;
  }

  memset(&addr, 0, sizeof(addr));
  addr.in.sin_family = AF_INET;
  addr.in.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.in.sin_port = htons(port);

  if (bind(listen_fd, &addr.sa, sizeof(addr)) < 0) {
    close(listen_fd);
    return NULL;
  }

  if (listen(listen_fd, 1) < 0) {
    close(listen_fd);
    return NULL;
  }

  conn_fd = accept(listen_fd, &addr.sa, &addr_len);
  close(listen_fd);
  if (conn_fd < 0) {
    return NULL;
  }

  peer = malloc(sizeof(*peer));
  if (peer == NULL) {
    close(conn_fd);
    return NULL;
  }

  peer->fd = conn_fd;
  return peer;
}

int peer_send(Peer *peer, const unsigned char *buf, size_t n) {
  size_t sent = 0;

  while (sent < n) {
    ssize_t result = send(peer->fd, buf + sent, n - sent, 0);
    if (result < 0) {
      return -1;
    }
    sent += (size_t)result;
  }

  return 0;
}

int peer_recv(Peer *peer, unsigned char *buf, size_t n) {
  size_t received = 0;

  while (received < n) {
    ssize_t result = recv(peer->fd, buf + received, n - received, 0);
    if (result <= 0) {
      return -1;
    }
    received += (size_t)result;
  }

  return 0;
}

void peer_close(Peer *peer) {
  if (peer == NULL) {
    return;
  }
  close(peer->fd);
  free(peer);
}

uint32_t decode_u32(const unsigned char *buf) {
  return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
         ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
}

int recv_config(Peer *peer, uint32_t *world_size, uint32_t *cycles,
                uint32_t *part_start, uint32_t *part_end) {
  unsigned char buf[16];
  if (peer_recv(peer, buf, 16) != 0)
    return -1;
  *world_size = decode_u32(buf);
  *cycles = decode_u32(buf + 4);
  *part_start = decode_u32(buf + 8);
  *part_end = decode_u32(buf + 12);
  return 0;
}

int send_config(Peer *peer, uint32_t world_size, uint32_t cycles,
                uint32_t part_start, uint32_t part_end) {
  unsigned char buf[16];
  buf[0] = (unsigned char)(world_size >> 24);
  buf[1] = (unsigned char)(world_size >> 16);
  buf[2] = (unsigned char)(world_size >> 8);
  buf[3] = (unsigned char)(world_size);
  buf[4] = (unsigned char)(cycles >> 24);
  buf[5] = (unsigned char)(cycles >> 16);
  buf[6] = (unsigned char)(cycles >> 8);
  buf[7] = (unsigned char)(cycles);
  buf[8] = (unsigned char)(part_start >> 24);
  buf[9] = (unsigned char)(part_start >> 16);
  buf[10] = (unsigned char)(part_start >> 8);
  buf[11] = (unsigned char)(part_start);
  buf[12] = (unsigned char)(part_end >> 24);
  buf[13] = (unsigned char)(part_end >> 16);
  buf[14] = (unsigned char)(part_end >> 8);
  buf[15] = (unsigned char)(part_end);
  return peer_send(peer, buf, 16);
}
