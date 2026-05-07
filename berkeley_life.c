/*
    Conway's Game of Life (and rough edges in the code for teaching purposes)
    Copyright (C) 2025 Nik Sultana

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#define _POSIX_C_SOURCE 200112L

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#ifndef ARGH
#include "args.h"
#endif
#ifndef PEERH
#include "peer.h"
#endif
#ifndef WORLDH
#include "world.h"
#endif

// set to avoid issues w/ TCP deadlocks in local WSL dev env
static void set_nodelay(Peer *peer) {
  int one = 1;
  setsockopt(peer->fd, IPPROTO_TCP, TCP_NODELAY, &one, (socklen_t)sizeof(one));
}

int main(int argc, char *const *argv) {
  Config cfg = parse_args(argc, argv);
  char *world_history = init_world(cfg.size, cfg.cycles, cfg.init_world);

  print_world(world_history, cfg.size, 0);
  printf("\n");

  // accept one connection per worker on consecutive ports
  Peer **peers = malloc(cfg.num_parts * sizeof(*peers));
  for (unsigned int i = 0; i < cfg.num_parts; i++) {
    peers[i] = peer_accept((unsigned short)((unsigned int)cfg.port + i));
    if (!peers[i]) {
      fprintf(stderr, "failed to accept worker %u\n", i);
      return EXIT_FAILURE;
    }
    set_nodelay(peers[i]);
  }

  // send each worker its config in one shot: world_size, cycles, part_start,
  // part_end
  unsigned long part_start = 0;
  for (unsigned int i = 0; i < cfg.num_parts; i++) {
    unsigned long part_end = part_start + cfg.parts[i];
    send_config(peers[i], (uint32_t)cfg.size, (uint32_t)cfg.cycles,
                (uint32_t)part_start, (uint32_t)part_end);
    part_start = part_end;
  }

  unsigned long step_size = cfg.size * cfg.size;

  for (unsigned long c = 1; c < cfg.cycles; c++) {
    char *cur = world_history + (c - 1) * step_size;
    char *nxt = world_history + c * step_size;

    // for each worker: send current step, then collect computed partition
    unsigned long ps = 0;
    for (unsigned int i = 0; i < cfg.num_parts; i++) {
      peer_send(peers[i], (const unsigned char *)cur, step_size);
      peer_recv(peers[i], (unsigned char *)(nxt + ps * cfg.size),
                cfg.parts[i] * cfg.size);
      ps += cfg.parts[i];
    }

    print_world(world_history, cfg.size, c);
    printf("\n");
  }

  for (unsigned int i = 0; i < cfg.num_parts; i++) {
    peer_close(peers[i]);
  }

  free(peers);
  free(world_history);
  free(cfg.init_world);
  free(cfg.parts);

  return EXIT_SUCCESS;
}
