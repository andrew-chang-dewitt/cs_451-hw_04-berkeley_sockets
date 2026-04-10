// fix implicit dprintf declaration
// from: https://stackoverflow.com/a/39671290
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORLDH

// setup step 0 state in world history
void init_step(char *world_history, const unsigned long size,
               const char *init_world);
// visualize world as rows (space separated) of 0s or 1s for each cell (comma
// separated w/in rows)
void print_world(int fd, char *world_history, const unsigned long size,
                 const unsigned long step_number);
// get value at (x,y) for step in history
char world_get_value(char *world_history, const unsigned long size,
                     const unsigned long step_number, const unsigned long x,
                     const unsigned long y);
// get state for step in history
void world_get_step(char *world_history, const unsigned long size,
                    const unsigned long step_number, char *step_ptr);
// set state for step in history
void world_set_step(char *world_history, const unsigned long size,
                    const unsigned long step_number, char *step);

// compute next state from pointer to given current state
// & store at given pointer
void step(char *cur_step, const unsigned long size, char *new_step);
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
