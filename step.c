#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>

#ifndef STEPH
#include "step.h"
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * private declarations
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// get value at (x,y) from given state
char step_get_value(char *step_state, const unsigned long size,
                    const unsigned long x, const unsigned long y);
// set value at (x,y) from given state
void step_set_value(char *step_state, const unsigned long size,
                    const unsigned long x, const unsigned long y,
                    const char value);
// get state of neighbor in direction of given offset from cell at (x,y)
char step_neighbour_state(char *step_state, const unsigned long size,
                          const unsigned long x, const unsigned long y,
                          const unsigned char neighbour_offset);
// count num living neighbours for cell (x,y) in given state
unsigned char step_living_neighbours(char *step_state, const unsigned long size,
                                     const unsigned long x,
                                     const unsigned long y);
// calculate the next value for the cell at the given step & coordinates
char cell_calc_next_val(char *cur_step, const unsigned long world_size,
                        unsigned long x, unsigned long y);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * public definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void step(char *cur_step, const unsigned long size, char *new_step) {
  for (unsigned long y = 0; y < size; y++) {
    for (unsigned long x = 0; x < size; x++) {
      char state = cell_calc_next_val(cur_step, size, x, y);

      step_set_value(new_step, size, x, y, state);
    }
  }
}

void step_part(char *cur_step, unsigned long part_start, unsigned long part_end,
               const unsigned long world_size, char *new_step) {
#undef VERBOSE // remove this exclusion to debug fn
#ifdef VERBOSE
  printf("\n[step_part()] BEGIN for partition starting @ %ld & ending @ %ld\n",
         part_start, part_end);
  printf("(%s:%d)\n\n", __FILE__, __LINE__);
#endif
  // for rows from partion start to partion end
  for (unsigned long y = part_start; y < part_end; y++) {
#ifdef VERBOSE
    printf("[step_part()] processing row %ld...\n  ", y);
#endif
    // for all cells in row
    for (unsigned long x = 0; x < world_size; x++) {
#ifdef VERBOSE
      char prev = step_get_value(cur_step, world_size, x, y);
      printf("%ld->", prev);
#endif
      char state = cell_calc_next_val(cur_step, world_size, x, y);

      step_set_value(new_step, world_size, x, y, state);
#ifdef VERBOSE
      printf("%ld(%ld,%ld), ", state, x, y);
#endif
    }
#ifdef VERBOSE
    printf("\n");
#endif
  }
#ifdef VERBOSE
  printf("\n[step_part()] END\n");
  printf("(%s:%d)\n\n", __FILE__, __LINE__);
#endif
#define VERBOSE // remove this exclusion to debug fn
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * private definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

char cell_calc_next_val(char *cur_step, const unsigned long world_size,
                        unsigned long x, unsigned long y) {
#undef VERBOSE
#ifdef VERBOSE
  printf("\n[cell_calc_next_val()] END\n");
  printf("(%s:%d)\n\n", __FILE__, __LINE__);
#endif
#define VERBOSE
  const unsigned char ln = step_living_neighbours(cur_step, world_size, x, y);
  char state = step_get_value(cur_step, world_size, x, y);
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

  return state;
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
