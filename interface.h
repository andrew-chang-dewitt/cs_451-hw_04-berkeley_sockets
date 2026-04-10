// fix implicit dprintf declaration
// from: https://stackoverflow.com/a/39671290
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>

#define INTERFACEH

/* * * * * * * * * * * * * * * * * * * * * * * *
 * data types
 * * * * * * * * * * * * * * * * * * * * * * * */
typedef struct {
  size_t len;
  char *buf;
} IntfData;

/* * * * * * * * * * * * * * * * * * * * * * * *
 * Child RPC
 * * * * * * * * * * * * * * * * * * * * * * * */
struct extension_data ext_child(struct extension_data data);

/* * * * * * * * * * * * * * * * * * * * * * * *
 * (De)Serializers
 * -------
 *
 * for inter-compart comms
 * * * * * * * * * * * * * * * * * * * * * * * */

// pack (serialize to bytes) a given step number (integer)
// & world history (size int & string of states)
#ifndef LC_ALLOW_EXCHANGE_FD
struct extension_data ext_step_to_arg(unsigned long size, char *step);
#else
struct extension_data ext_step_to_arg(unsigned long size, char *step, int fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD

// unpack a step number (integer), size (integer), & world history (string of
// states) from a given argument & store each in given pointers
// data {
//   //     8 bytes   8 bytes  4 bytes       size_history bytes
//   //     ul        ul       size_t        char*
//   buf: { step_num, size   , size_history, world_history }
// }
#ifndef LC_ALLOW_EXCHANGE_FD
void ext_step_from_arg(struct extension_data data, unsigned long *size_ptr,
                       char *step_state_ptr);
#else
void ext_step_from_arg(struct extension_data data, unsigned long *size_ptr,
                       char *step_state_ptr, int *fd);
#endif // ndef LC_ALLOW_EXCHANGE_FD

char *packbuf(char *buf, char *data, size_t len);
char *unpackbuf(char *buf, char *data, size_t len);
