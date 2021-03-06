#ifndef __SOCKTABLE_H__
#define __SOCKTABLE_H__

#include <htable.h>
#include <stdint.h>
#include "tcp.h"

#define HALF_SOCKET   0
#define FULL_SOCKET   1

struct tcp_socket__;

typedef struct socksplit__ {
  htable_t* full;
  struct tcp_socket__* half;
} socksplit_t;

typedef struct socktable__ {
  htable_t root;
} socktable_t;

void socktable_init(socktable_t *st);
void socktable_destroy(socktable_t *st);
void socktable_dump(socktable_t *st, uint8_t full);

struct tcp_socket__ *socktable_get(socktable_t *st, int lnode, uint16_t lport, int rnode, uint16_t rport, uint8_t full);
struct tcp_socket__ *socktable_put(socktable_t *st, struct tcp_socket__ *data, uint8_t full);
struct tcp_socket__ *socktable_remove(socktable_t *st, int lnode, uint16_t lport, int rnode, uint16_t rport, uint8_t full);
struct tcp_socket__ *socktable_promote(socktable_t *st, struct tcp_socket__ *data);

#define socktable_full_iterate_begin(st, var) \
do { \
  htable_t *__st_lph, *__st_rph; \
  socksplit_t *__st_rns; \
  htable_iterate_begin(&st->root, __st_lph, htable_t) { \
    htable_iterate_begin(__st_lph, __st_rns, socksplit_t) { \
      if(!__st_rns->full) { \
        continue; \
      } \
      htable_iterate_begin(__st_rns->full, __st_rph, htable_t) { \
        htable_iterate_begin(__st_rph, var, struct tcp_socket__) {

#define socktable_full_iterate_end() \
        } htable_iterate_end(); \
      } htable_iterate_end(); \
    } htable_iterate_end(); \
  } htable_iterate_end(); \
} while(0)

#define socktable_half_iterate_begin(st, var) \
do { \
  htable_t *__st_lph, *__st_rph; \
  socksplit_t *__st_rns; \
  htable_iterate_begin(&st->root, __st_lph, htable_t) { \
    htable_iterate_begin(__st_lph, __st_rns, socksplit_t) { \
      if(!(var = (struct tcp_socket__*)__st_rns->half)) { \
        continue; \
      } \
  
#define socktable_half_iterate_end() \
    } htable_iterate_end(); \
  } htable_iterate_end(); \
} while(0)

#endif
