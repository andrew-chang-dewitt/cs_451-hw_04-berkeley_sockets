#define _POSIX_C_SOURCE 200112L

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#ifndef PEERH
#include "peer.h"
#endif
#ifndef STEPH
#include "step.h"
#endif

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *host = argv[1];
  unsigned short port = (unsigned short)strtoul(argv[2], NULL, 10);

  // retry connecting — main accepts workers sequentially, so later workers
  // must wait for earlier ones to connect first
  Peer *main_node = NULL;
  for (int attempt = 0; attempt < 20 && !main_node; attempt++) {
    if (attempt > 0) {
      struct timespec ts = {0, 100000000L}; /* 100 ms */
      nanosleep(&ts, NULL);
    }
    main_node = peer_connect(host, port);
  }
  if (!main_node) {
    fprintf(stderr, "failed to connect to %s:%hu\n", host, port);
    return EXIT_FAILURE;
  }

  int one = 1;
  setsockopt(main_node->fd, IPPROTO_TCP, TCP_NODELAY, &one,
             (socklen_t)sizeof(one));

  uint32_t world_size, cycles, part_start, part_end;
  if (recv_config(main_node, &world_size, &cycles, &part_start, &part_end) !=
      0) {
    fprintf(stderr, "failed to receive config\n");
    peer_close(main_node);
    return EXIT_FAILURE;
  }

  unsigned long step_size =
      (unsigned long)world_size * (unsigned long)world_size;
  unsigned long part_rows = (unsigned long)(part_end - part_start);

  char *cur_step = malloc(step_size);
  char *new_step = malloc(step_size);
  if (!cur_step || !new_step) {
    fprintf(stderr, "malloc failed\n");
    peer_close(main_node);
    return EXIT_FAILURE;
  }

  for (uint32_t c = 1; c < cycles; c++) {
    if (peer_recv(main_node, (unsigned char *)cur_step, step_size) != 0)
      break;

    step_part(cur_step, (unsigned long)part_start, (unsigned long)part_end,
              (unsigned long)world_size, new_step);

    if (peer_send(
            main_node,
            (const unsigned char *)(new_step + (unsigned long)part_start *
                                                   (unsigned long)world_size),
            part_rows * (unsigned long)world_size) != 0)
      break;
  }

  free(cur_step);
  free(new_step);
  peer_close(main_node);

  return EXIT_SUCCESS;
}
