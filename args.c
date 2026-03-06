#include <assert.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARGH
#include "args.h"
#endif

const Config default_config = {
    .size = 5, .cycles = 5, .num_parts = 4,
    // .parts is NULL
    // .init_world is NULL
};

Config parse_args(const int argc, char *const *argv) {
  Config config = default_config;

  int option;
  while ((option = getopt(argc, argv, "s:i:c:g:")) != -1) {
    switch (option) {
    case 'i': // initial state
      config.init_world = malloc(strlen(optarg) + 1);
      strcpy(config.init_world, optarg);
      break;
    case 's': // size (dimension of square world)
      config.size = strtoul(optarg, NULL, 10);
      break;
    case 'c': // number of iteration to run
      config.cycles = strtoul(optarg, NULL, 10);
      break;
    case 'g': // how many parts to break each step into
      config.num_parts = (unsigned int)strtoul(optarg, NULL, 10);
      break;
    default:
      // FIXME: print error message.
      exit(EXIT_FAILURE);
      break;
    }
  }

  assert(config.size > 1);
  assert(config.cycles > 1);

  // exit error more parts than rows/columns in world
  assert(config.size >= config.num_parts);
  // calculate size of each part
  config.parts = malloc(config.num_parts * sizeof(*config.parts));

  if (1 == config.num_parts) {
    // 1 part means entire world in 1 step:
    *config.parts = config.size;
  } else {
    // for a world of size S split into N parts,
    // there will be S % N parts with (S / N) + 1 rows &
    // N - (S % N) parts w/ S / N rows
    unsigned long rem = config.size % config.num_parts;

    // start w/ (S / N) + 1 rows
    unsigned long part_size = config.size / config.num_parts + 1;
    // iterating one part at a time from beginning of parts array
    unsigned long *next_part = config.parts;
    // until all remainder rows have been used
    unsigned long *end = config.parts + rem;
    for (; next_part < end; next_part++) {
      *next_part = part_size;
    }

    // then do remaining N - (S % N) rows
    end = config.parts + config.num_parts;
    part_size -= 1;
    for (; next_part < end; next_part++) {
      *next_part = part_size;
    }
  }

#ifdef VERBOSE
  printf("[parse_args()]\n");
  printf("  Config built: {\n");
  printf("    .size = %ld,\n", config.size);
  printf("    .cycles = %ld,\n", config.cycles);
  printf("    .num_parts = %ld,\n", config.num_parts);
  printf("    .parts = [ ");
  for (unsigned long i = 0; i < config.num_parts; i++) {
    printf("%ld, ", config.parts[i]);
  }
  printf("]\n");
  printf("    .init_world = %s,\n", config.init_world);
  printf("  }\n");
  printf("(%s:%d)\n\n", __FILE__, __LINE__);
#endif
  return config;
}
