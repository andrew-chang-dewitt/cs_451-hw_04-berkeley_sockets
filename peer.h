// abstractions on TCP connections to simplify sending step parts
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

// example usage:
//
// description: main node that 4 peer nodes will need to establish a connection
// to, then the main node will run a loop that sends a portion of a row in a 2d
// array to each node, then waits for a response from all 4 peers before moving
// on to the next row.
//
// `peer_accept` blocks for one connection at a time, so we call it four times
// in sequence to collect all four peer handles before starting work.
//
// ```c
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "peer.h"
//
// #define NUM_PEERS   4
// #define GRID_SIZE   16                     /* NxN grid */
// #define CHUNK_COLS  (GRID_SIZE / NUM_PEERS) /* columns per peer */
// #define BASE_PORT   9000
//
// int main(void) {
//     /* --- accept one connection per peer node on consecutive ports --- */
//     Peer *peers[NUM_PEERS];
//     for (int i = 0; i < NUM_PEERS; i++) {
//         peers[i] = peer_accept((unsigned short)(BASE_PORT + i));
//         if (peers[i] == NULL) {
//             fprintf(stderr, "failed to accept peer %d\n", i);
//             return EXIT_FAILURE;
//         }
//     }
//
//     /* a flat row-major 2-D grid of bytes (values 0 or 1) */
//     unsigned char grid[GRID_SIZE][GRID_SIZE];
//     memset(grid, 0, sizeof(grid));
//     /* ... populate grid with initial state ... */
//
//     unsigned char response[CHUNK_COLS]; /* each peer returns its updated
//     chunk */
//
//     /* --- iterate over rows --- */
//     for (int row = 0; row < GRID_SIZE; row++) {
//         /* send each peer its slice of the current row */
//         for (int p = 0; p < NUM_PEERS; p++) {
//             unsigned char *chunk = &grid[row][p * CHUNK_COLS];
//             if (peer_send(peers[p], chunk, CHUNK_COLS) < 0) {
//                 fprintf(stderr, "send failed for peer %d row %d\n", p, row);
//                 return EXIT_FAILURE;
//             }
//         }
//
//         /* wait for the updated chunk back from every peer before moving on
//         */ for (int p = 0; p < NUM_PEERS; p++) {
//             if (peer_recv(peers[p], response, CHUNK_COLS) < 0) {
//                 fprintf(stderr, "recv failed for peer %d row %d\n", p, row);
//                 return EXIT_FAILURE;
//             }
//             /* write the result back into the grid */
//             memcpy(&grid[row][p * CHUNK_COLS], response, CHUNK_COLS);
//         }
//     }
//
//     /* --- tear down --- */
//     for (int i = 0; i < NUM_PEERS; i++) {
//         peer_close(peers[i]);
//     }
//
//     return EXIT_SUCCESS;
// }
// ```
//
// And on each **peer node** the matching side is just:
//
// ```c
// Peer *main_node = peer_connect("10.0.0.1", (unsigned short)(BASE_PORT +
// MY_PEER_ID));
//
// for (int row = 0; row < GRID_SIZE; row++) {
//     unsigned char chunk[CHUNK_COLS];
//     peer_recv(main_node, chunk, CHUNK_COLS);   /* receive slice */
//     /* ... compute updated values ... */
//     peer_send(main_node, chunk, CHUNK_COLS);   /* send result back */
// }
//
// peer_close(main_node);
// ```
//
// A few things worth noting for when this evolves:
// - The `peer_accept` calls on the main node are sequential right now — once
//   we need them to connect in any order a listening loop with `select`/`poll`
//   (or pthreads, given the rest of the project) will be cleaner.
// - `peer_send`/`peer_recv` already loop internally until all `n` bytes have
//   been transferred, so partial TCP writes/reads are handled transparently.
