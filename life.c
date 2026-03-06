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


gcc life.c -o life # -pg
valgrind -s --leak-check=full --show-leak-kinds=all ./life -s 20 -c 20 -i
010000000001000000000010000000010000000011100000000100000000 > life.out
./life -s 3 -c 3 -i 011001010
./life -s 5 -c 10 -i 011001010110101111
./life -s 20 -c 10 -i
011001010110101111011001010110101111010111101010101111100110111 Glider and
blinker:
./life -s 20 -c 20 -i
010000000001000000000010000000010000000011100000000100000000 Bee-hive and loaf:
./life -s 10 -c 100 -i
010000000001000000000010000000010000000011100000000100000000 > life.out
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef ARGH
#include "args.h"
#endif
#ifndef WORLDH
#include "world.h"
#endif
#ifndef STEPH
#include "step.h"
#endif

int main(int argc, char *const *argv) {
  Config cfg = parse_args(argc, argv);
  char *world_history = init_world(cfg.size, cfg.cycles, cfg.init_world);

  print_world(world_history, cfg.size, 0);
  printf("\n");

  for (unsigned long i = 1; i < cfg.cycles; i++) {
    step(world_history + (i - 1) * cfg.size * cfg.size, cfg.size,
         world_history + i * cfg.size * cfg.size);
    print_world(world_history, cfg.size, i);
    printf("\n");
  }

  free(world_history);
  free(cfg.init_world);

  return EXIT_SUCCESS;
}
