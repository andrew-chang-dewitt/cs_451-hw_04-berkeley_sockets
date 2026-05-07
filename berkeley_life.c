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
valgrind -s --leak-check=full --show-leak-kinds=all \
  ./life -s 20 -c 20 \
  -i 010000000001000000000010000000010000000011100000000100000000

./life -s 3 -c 3 -i 011001010

./life -s 5 -c 10 -i 011001010110101111

./life -s 20 -c 10 \
  -i 011001010110101111011001010110101111010111101010101111100110111

# Glider and blinker:
./life -s 20 -c 20 \
  -i 010000000001000000000010000000010000000011100000000100000000

# Bee-hive and loaf:
./life -s 10 -c 100 \
  -i 010000000001000000000010000000010000000011100000000100000000
*/
// fix error w/ unknown * ‘pthread_barrier_*’ caused by -std=c99 compile time
// flag
#define _POSIX_C_SOURCE 200112L /* Or higher */
// Source - https://stackoverflow.com/a/61648098
// Posted by Shawn, modified by community. See post 'Timeline' for change
// history Retrieved 2026-02-20, License - CC BY-SA 4.0

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef ARGH
#include "args.h"
#endif
#ifndef STEPH
#include "step.h"
#endif
#ifndef WORLDH
#include "world.h"
#endif

struct worker_args {
#ifdef VERBOSE
  unsigned int part_num;
#endif
  unsigned long part_start;
  unsigned long part_end;
  unsigned long cycles;
  unsigned long world_size;
  pthread_barrier_t *barrier;
  char *world_history;
};

void *thread_worker(void *void_args) {
#ifdef VERBOSE
  printf("\n[thread_worker()] BEGIN for partition @ %d\n",
         (*(struct worker_args *)void_args).part_num);
  printf("(%s:%d)\n\n", __FILE__, __LINE__);
#endif
  struct worker_args args = *(struct worker_args *)void_args;

  char *cur_step;
  char *new_step = args.world_history;

  for (unsigned long i = 1; i < args.cycles; i++) {
    // move step pointers ahead one step
    cur_step = new_step;
    new_step = cur_step + (args.world_size * args.world_size);

    // calculate this step, updating own portion of shared history state
    step_part(cur_step, args.part_start, args.part_end, args.world_size,
              new_step);

    // wait for other threads to complete step before moving on
    pthread_barrier_wait(args.barrier);
  }
#ifdef VERBOSE
  printf("\n[thread_worker()] END for partition @ %d\n", args.part_num);
  printf("(%s:%d)\n\n", __FILE__, __LINE__);
#endif
  return NULL;
}

int main(int argc, char *const *argv) {
  // parse args to config values
  Config cfg = parse_args(argc, argv);
  // initialize state obj
  char *world_history = init_world(cfg.size, cfg.cycles, cfg.init_world);

  // print initial state as first step
  print_world(world_history, cfg.size, 0);
  printf("\n");

  // set up thread sync primatives
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, cfg.num_parts + 1);
  // and thread worker variables
  unsigned long part_start;
  unsigned long part_end = 0;
  pthread_t threads[cfg.num_parts];
  struct worker_args thread_args[cfg.num_parts];

  // configure workers for each part & start a thread for each
  for (unsigned int t = 0; t < cfg.num_parts; t++) {
    part_start = part_end;
    part_end = part_start + cfg.parts[t];

    thread_args[t] = (struct worker_args){
#ifdef VERBOSE
        .part_num = t,
#endif
        .part_start = part_start,
        .part_end = part_end,
        .cycles = cfg.cycles,
        .world_size = cfg.size,
        .barrier = &barrier,
        .world_history = world_history,
    };
#ifdef VERBOSE
    printf("\n[main()] starting thread for partition %d\n", t);
    printf("(%s:%d)\n\n", __FILE__, __LINE__);
#endif

    pthread_create(&threads[t], NULL, &thread_worker, (void *)&thread_args[t]);
  }

  // main thread will print each step
  for (unsigned long i = 1; i < cfg.cycles; i++) {
    // wait for threads to signal a step is complete
    pthread_barrier_wait(&barrier);
    // then print that step
    print_world(world_history, cfg.size, i);
    printf("\n");
  }

  // wait for threads to close
  for (unsigned int t = 0; t < cfg.num_parts; t++) {
    pthread_join(threads[t], NULL);
#ifdef VERBOSE
    printf("\n[main()] thread for partition %d ended\n", t);
    printf("(%s:%d)\n\n", __FILE__, __LINE__);
#endif
  }

  free(world_history);
  free(cfg.init_world);
  free(cfg.parts);

  return EXIT_SUCCESS;
}
