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

#ifndef WORLDH
#include "world.h"
#endif

void print_world(int fd, char *world_history, const unsigned long size,
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
  for (unsigned long y = 0; y < size; y++) {
    for (unsigned long x = 0; x < size; x++) {
      char v = world_get_value(world_history, size, step_number, x, y);
      char terminator[1] = "";
      if (x < size - 1)
        terminator[0] = ',';
      dprintf(fd, "%d%s", v, terminator);
    }
    dprintf(fd, " ");
  }
}

void init_step(char *world_history, const unsigned long size,
               const char *init_world) {
  bool completely_uninitialized = false;
  unsigned long init_size = 0;
  if (NULL != init_world)
    init_size = strlen(init_world);
  else
    completely_uninitialized = true;

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
      return;
    } else {
      fprintf(stderr,
              "Invalid initial state"); // FIXME: show the offending character.
      exit(EXIT_FAILURE);
    }
  }
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

void step(char *cur_step, const unsigned long size, char *new_step) {
  // void step(char *world_history, const unsigned long size,
  //           const unsigned long step_number) {
  for (unsigned long y = 0; y < size; y++) {
    for (unsigned long x = 0; x < size; x++) {
      const unsigned char ln = step_living_neighbours(cur_step, size, x, y);
      char state = step_get_value(cur_step, size, x, y);
      if (1 == state) {
        if (ln < 2) {
          state = 0;
        } else if (2 == ln || 3 == ln) {
          state = 1;
        } else if (ln > 3) {
          state = 0;
        }
      } else if (0 == state) {
        if (3 == ln) {
          state = 1;
        }
      }

      step_set_value(new_step, size, x, y, state);
    }
  }
}

char step_get_value(char *step_state, const unsigned long size,
                    const unsigned long x, const unsigned long y) {
  // FIXME assert x >= 0
  // FIXME assert y >= 0
  // FIXME assert x < size
  // FIXME assert y < size
  return step_state[y * size + x];
}

void step_set_value(char *step_state, const unsigned long size,
                    const unsigned long x, const unsigned long y,
                    const char value) {
  // FIXME assert x >= 0
  // FIXME assert y >= 0
  // FIXME assert x < size
  // FIXME assert y < size
  step_state[y * size + x] = value;
}

char step_neighbour_state(char *step_state, const unsigned long size,
                          const unsigned long x, const unsigned long y,
                          const unsigned char neighbour_offset) {
  /* Numberings of the neighbours of the cell labeled "C":
        _ _ _
       |0|1|2|
       |-|-|-|
       |3|C|4|
       |-|-|-|
       |5|6|7|
        - - -
  */
  char result = (char)-2;

  switch (neighbour_offset) {
  case 0:
    if (0 == x || 0 == y) {
      result = (char)-1;
    } else {
      result = step_get_value(step_state, size, x - 1, y - 1);
    }
    break;
  case 1:
    if (0 == y) {
      result = (char)-1;
    } else {
      result = step_get_value(step_state, size, x, y - 1);
    }
    break;
  case 2:
    if (size - 1 == x || 0 == y) {
      result = (char)-1;
    } else {
      result = step_get_value(step_state, size, x + 1, y - 1);
    }
    break;
  case 3:
    if (0 == x) {
      result = (char)-1;
    } else {
      result = step_get_value(step_state, size, x - 1, y);
    }
    break;
  case 4:
    if (size - 1 == x) {
      result = (char)-1;
    } else {
      result = step_get_value(step_state, size, x + 1, y);
    }
    break;
  case 5:
    if (0 == x || size - 1 == y) {
      result = (char)-1;
    } else {
      result = step_get_value(step_state, size, x - 1, y + 1);
    }
    break;
  case 6:
    if (size - 1 == y) {
      result = (char)-1;
    } else {
      result = step_get_value(step_state, size, x, y + 1);
    }
    break;
  case 7:
    if (size - 1 == x || size - 1 == y) {
      result = (char)-1;
    } else {
      result = step_get_value(step_state, size, x + 1, y + 1);
    }
    break;
  default:
    // FIXME terminate with error message.
    break;
  }

  return result;
}

unsigned char step_living_neighbours(char *step_state, const unsigned long size,
                                     const unsigned long x,
                                     const unsigned long y) {
  // FIXME assert x >= 0
  // FIXME assert y >= 0
  // FIXME assert x < size
  // FIXME assert y < size
  unsigned char result = 0;

  for (unsigned char i = 0; i < 8; i++) {
    if (1 == step_neighbour_state(step_state, size, x, y, i)) {
      result += 1;
    }
  }

  return result;
}
