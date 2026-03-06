#define ARGH

typedef struct {
  unsigned long size;
  unsigned long cycles;
  unsigned int num_parts;
  unsigned long *parts;
  char *init_world;
} Config;

Config parse_args(int argc, char *const *argv);
