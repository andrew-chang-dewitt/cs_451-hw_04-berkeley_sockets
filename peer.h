#define PEERH

#include <stddef.h>

typedef struct {
  int fd;
} Peer;

// connect to a remote peer at host:port; returns NULL on failure
Peer *peer_connect(const char *host, unsigned short port);
// listen on port and accept one incoming connection; returns NULL on failure
Peer *peer_accept(unsigned short port);
// send exactly n bytes from buf to peer; returns 0 on success, -1 on error
int peer_send(Peer *peer, const unsigned char *buf, size_t n);
// receive exactly n bytes from peer into buf; returns 0 on success, -1 on error
int peer_recv(Peer *peer, unsigned char *buf, size_t n);
// close the connection and free the Peer
void peer_close(Peer *peer);
