// fix implicit dprintf declaration
// from: https://stackoverflow.com/a/39671290
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef WORLDH
#include "world.h"
#endif

#ifndef INTERFACEH
#include "interface.h"
#endif

/* * * * * * * * * * * * * * * * * * * * * * * *
 * Globals
 * -------
 *
 * includes comparts map, extension id,
 * & world history state
 *
 * declared as extern to be available to all
 * compartments
 * * * * * * * * * * * * * * * * * * * * * * * */

struct compart comparts[NO_COMPARTS] = {{.name = "main compartment",
                                         .uid = 65534,
                                         .gid = 65534,
                                         .path = "/tmp",
                                         .comms = NULL},
                                        {.name = "step compartment",
                                         .uid = 65534,
                                         .gid = 65534,
                                         .path = "/tmp",
                                         .comms = NULL}};

// init public extension identifer pointers as NULL
// they must be redefined by user of this code
struct extension_id *step_ext = NULL;

/* * * * * * * * * * * * * * * * * * * * * * * *
 * Compartment extension fn
 * * * * * * * * * * * * * * * * * * * * * * * */

// execute next step based on the given state
// shows pattern we'll see across other ext_* functions:
// 1. declare a file descriptor for logging
// 2. deserialize argument data
// 3. perform computation w/ output of (2)
// 4. serialize value to return
// 5. return output of (4)
//
// data {
//   //     8 bytes  size_history bytes
//   //     ul       char*
//   buf: { size   , step state }
// }
struct extension_data ext_step(struct extension_data data) {
  // 1. declare a file descriptor for logging
  int fd = -1;
#ifdef LC_ALLOW_EXCHANGE_FD
  fd = STDOUT_FILENO;
#endif
  // dprintf(fd, "[%s:ext_step()]:%d BEGIN\n", compart_name(), getuid());

  // 2. deserialize argument data
  struct extension_data result;
  result.bufc = data.bufc;

  unsigned long size = 0;
  size_t size_step = data.bufc - sizeof(size);
  char *prev_step = malloc(size_step);

#ifndef LC_ALLOW_EXCHANGE_FD
  ext_step_from_arg(data, &size, prev_step);
#else
  ext_step_from_arg(data, &size, prev_step, fd);
#endif
  // dprintf(fd, "[%s:ext_step()]:%d state extracted for step of size (%ld)\n",
  //         compart_name(), getuid(), size);

  // 3. perform computation w/ output of (2)
  char *next_step = malloc(size_step);
  step(prev_step, size, next_step);

  // 4. serialize value to return
  result = ext_step_to_arg(size, next_step);

  // 5. return output of (4)
  free(prev_step);
  free(next_step);

  return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * *
 * (De)Serializers
 * -------
 *
 * for inter-compart comms
 * * * * * * * * * * * * * * * * * * * * * * * */

// pack (serialize to bytes) a size (integer)
// & step (string of states)
// data {
//   //     8 bytes  size_history bytes
//   //     ul       char*
//   buf: { size   , world_history }
// }
#ifndef LC_ALLOW_EXCHANGE_FD
struct extension_data ext_step_to_arg(unsigned long size, char *step)
#else
struct extension_data ext_step_to_arg(unsigned long size, char *step, int fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  // create empty result value
  struct extension_data result;

  // declare size of buffer
  size_t size_size = sizeof(size);
  size_t size_step = size * size;
  result.bufc = size_size + size_step;

  char *cur = packbuf(result.buf, (char *)&size, size_size);
  packbuf(cur, step, size_step);

#ifdef LC_ALLOW_EXCHANGE_FD
  result.fdc = 1;
  printf("(%d) ext_step_to_arg fd=%d\n", getpid(), fd);
  result.fd[0] = fd;
#endif // LC_ALLOW_EXCHANGE_FD
  // return result object
  return result;
}

// unpack a size (integer) from a given argument & store each in given pointers
// data {
//   //     8 bytes  size_history bytes
//   //     ul       char*
//   buf: { size   , world_history }
// }
#ifndef LC_ALLOW_EXCHANGE_FD
void ext_step_from_arg(struct extension_data data, unsigned long *size_ptr,
                       char *step_state_ptr)
#else
void ext_step_from_arg(struct extension_data data, unsigned long *size_ptr,
                       char *step_state_ptr, int *fd)
#endif // ndef LC_ALLOW_EXCHANGE_FD
{
  size_t size_size = sizeof(*size_ptr);

  char *cur = unpackbuf(data.buf, (char *)size_ptr, size_size);

  size_t size_state = data.bufc - size_size;

  unpackbuf(cur, step_state_ptr, size_state);

#ifdef LC_ALLOW_EXCHANGE_FD
  // FIXME assert result.fdc == 1;
  *fd = data.fd[0];
#endif // LC_ALLOW_EXCHANGE_FD
}

char *packbuf(char *buf, char *data, size_t len) {
  memcpy(buf, data, len);

  return buf + len;
}

char *unpackbuf(char *buf, char *data, size_t len) {
  memcpy(data, buf, len);

  return buf + len;
}
