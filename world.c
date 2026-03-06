#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef WORLDH
#include "world.h"
#endif

char *init_world(unsigned long size, unsigned long cycles, char *init_world) {
  bool completely_uninitialized = false;
  unsigned long init_size = 0;
  if (NULL != init_world)
    init_size = strlen(init_world);
  else
    completely_uninitialized = true;

  char *world_history = malloc(size * size * cycles);
  for (unsigned long i = 0; i < size * size * cycles; i++) {
    world_history[i] = 0;
  }

  for (unsigned long i = 0; i < size * size; i++) {
    if (completely_uninitialized || i >= init_size) {
      world_history[i] = 0;
      continue;
    }

    if (0 == strncmp("0", init_world + i, 1)) {
      world_history[i] = 0;
    } else if (0 == strncmp("1", init_world + i, 1)) {
      world_history[i] = 1;
    } else if (0 == strncmp("\0", init_world + i, 1)) {
      return world_history;
    } else {
      fprintf(stderr,
              "Invalid initial state"); // FIXME: show the offending character.
      exit(EXIT_FAILURE);
    }
  }

  return world_history;
}

void print_world(char *world_history, const unsigned long size,
                 const unsigned long step_number) {
  /*
    for (int y = 0; y < size; y++) {
      for (int x = 0; x < size; x++) {
        int v = get_value(size, step_number, x, y);
        printf("%d", v);
      }
      printf("\n");
    }
  */
#ifdef PRETTY
  for (unsigned long y = 0; y < size; y++) {
    for (unsigned long x = 0; x < size; x++) {
      char v = world_get_value(world_history, size, step_number, x, y);
      if (1 == (int)v)
        printf("▩");
      else
        printf("·");
      if (x < size - 1)
        printf(" ");
    }
    printf("\n");
  }
  printf("\n");
#else
  for (unsigned long y = 0; y < size; y++) {
    for (unsigned long x = 0; x < size; x++) {
      char v = world_get_value(world_history, size, step_number, x, y);
      printf("%d", v);
      if (x < size - 1)
        printf(",");
    }
    printf(" ");
  }
#endif
}

char world_get_value(char *world_history, const unsigned long size,
                     const unsigned long step_number, const unsigned long x,
                     const unsigned long y) {
  // FIXME assert x >= 0
  // FIXME assert y >= 0
  // FIXME assert x < size
  // FIXME assert y < size
  return world_history[size * size * step_number + y * size + x];
}

void world_get_step(char *world_history, const unsigned long size,
                    const unsigned long step_number, char *step_ptr) {
  const unsigned long offset = step_number * size * size;

  for (unsigned long i = 0; i < size * size; i++) {
    // copy value at ith cell in step
    step_ptr[i] = world_history[i + offset];
  }
}

void world_set_step(char *world_history, const unsigned long size,
                    const unsigned long step_number, char *step) {
  const unsigned long offset = step_number * size * size;

  for (unsigned long i = 0; i < size * size; i++) {
    // copy value at ith cell in step
    world_history[i + offset] = step[i];
  }
}
