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
// fix implicit dprintf declaration
// from: https://stackoverflow.com/a/39671290
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef INTERFACEH
#include "interface.h"
#endif

#ifndef WORLDH
#include "world.h"
#endif

// int main(int argc, char *const *argv) {
//   struct extension_data data = ext_ints_to_arg(2, 3);
//   int num1 = -1;
//   int num2 = -1;
//   ext_ints_from_arg(data, &num1, &num2);
//
//   if (num1 != 2) {
//     printf("num1 should be 2, got %d\n", num1);
//     return EXIT_FAILURE;
//   }
//   if (num2 != 3) {
//     printf("num2 should be 3, got %d\n", num2);
//     return EXIT_FAILURE;
//   }
//
//   printf("success, got (%d, %d)\n", num1, num2);
//
//   return EXIT_SUCCESS;
// }

int main(int argc, char *const *argv) {
  compart_check();
  compart_init(NO_COMPARTS, comparts, default_config);
  // register pointer to function extension calculate step on step compartment
  step_ext = compart_register_fn("step compartment", &ext_child);
  compart_start("main compartment");

  // get a handle on stdout
#ifndef LC_ALLOW_EXCHANGE_FD
  int fd = STDOUT_FILENO;
#else
  int fd = open("stdout", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  dprintf(fd, "(%d) open fd=%d\n", getpid(), fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD
       //
  unsigned long size = 3;
  char *init_world = NULL;
  unsigned long cycles = 3;

  int option;
  while ((option = getopt(argc, argv, "s:i:c:")) != -1) {
    switch (option) {
    case 'i':
      init_world = malloc(strlen(optarg) + 1);
      strcpy(init_world, optarg);
      break;
    case 's':
      size = strtoul(optarg, NULL, 10);
      break;
    case 'c':
      cycles = strtoul(optarg, NULL, 10);
      break;
    default:
      // FIXME: print error message.
      exit(EXIT_FAILURE);
      break;
    }
  }

  assert(size > 1);
  assert(cycles > 1);

  size_t size_history = size * size * cycles + 1;
  char *world_history = malloc(size_history);

  for (unsigned long i = 0; i < size * size * cycles; i++) {
    world_history[i] = 0;
  }

  init_step(world_history, size, init_world);
  print_world(fd, world_history, size, 0);
  dprintf(fd, "\n");

  size_t size_step = size * size + 1;
  char *step_state = malloc(size_step);
  struct extension_data req;
  struct extension_data res;

  unsigned long step_num = 1;

  for (; step_num < cycles; step_num++) {
    // serialize previous step
    world_get_step(world_history, size, step_num - 1, step_state);
    req = ext_step_to_arg(size, step_state);
    // call in step compartment
    res = compart_call_fn(step_ext, req);
    // extract newly computed step & place @ step_state
    ext_step_from_arg(res, &size, step_state);
    // then update history w/ this step
    world_set_step(world_history, size, step_num, step_state);

    print_world(fd, world_history, size, step_num);
    dprintf(fd, "\n");
  }

  // cleanup
  free(world_history);
  free(init_world);
  free(step_state);
  // free(step_ext);
  close(fd);

  return EXIT_SUCCESS;
}
