#ifndef __SOCKTABLE_H__
#define __SOCKTABLE_H__

#include <htable.h>
#include "tcp.h"

typedef htable_t socktable_t;

void socktable_init(socktable_t *st);
void socktable_destroy(socktable_t *st);
struct tcp_socket__ *socktable_get(socktable_t *st, int lnode, short lport, int rnode, short rport);
struct tcp_socket__ *socktable_put(socktable_t *st, struct tcp_socket__ *data);
struct tcp_socket__ *socktable_remove(socktable_t *st, int lnode, short lport, int rnode, short rport);
void socktable_dump(socktable_t *st);

#define socktable_iterate_begin(st, var) \
do { \
  htable_t *__st_lph, *__st_rnh, *__st_rph; \
  htable_iterate_begin(st, __st_lph, htable_t) { \
    htable_iterate_begin(__st_lph, __st_rnh, htable_t) { \
      htable_iterate_begin(__st_rnh, __st_rph, htable_t) { \
        htable_iterate_begin(__st_rph, var, struct tcp_socket__) {

#define socktable_iterate_end() \
        } htable_iterate_end(); \
      } htable_iterate_end(); \
    } htable_iterate_end(); \
  } htable_iterate_end(); \
} while(0)

#endif
