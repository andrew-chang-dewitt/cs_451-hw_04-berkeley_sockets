#define WORLDH

// setup world history state obj
char *init_world(unsigned long size, unsigned long cycles, char *init_world);
// visualize world as rows (space separated) of 0s or 1s for each cell (comma
// separated w/in rows)
void print_world(char *world_history, const unsigned long size,
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
