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
