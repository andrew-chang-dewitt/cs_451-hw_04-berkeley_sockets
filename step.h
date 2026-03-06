#define STEPH

// compute next state from pointer to given current state
// & store at given pointer
void step(char *cur_step, const unsigned long size, char *new_step);
// multithreading-ready step calculation; distinguishes between world size &
// part size (number of rows in the partition) to know when done w/ part
void step_part(char *cur_step, unsigned long part_start, unsigned long part_end,
               const unsigned long world_size, char *new_step);
