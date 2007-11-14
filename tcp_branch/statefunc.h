#ifndef __STATEFUNC_H_
#define __STATEFUNC_H_

#include "tcp.h"
#include "state.h"

#define CLOSE_ERROR	0x01	// Close is communicated as error return
#define CLOSE_EOF		0x02	// Close is communicated as EOF
#define CLOSE_OK		0x04	// Close is communicated as success
#define CLOSE_NIL		0x08  // Close requires no communication

struct tcp_socket__;

int do_send_flags (sid_t prev, sid_t next, void* context, void *arg, void *tran_arg);
int do_listen(sid_t prev, sid_t next, void* context, void *arg, void *tran_arg);
int wait_for_event(struct tcp_socket__ *sock, int status_bits);
void fail_with_reset(sid_t, void*, void*);
int do_connect(sid_t prev, sid_t next, void* context, void* arg, void* tran_arg);
int do_close(sid_t prev, sid_t next, void* context, void* argA, void* argB);
void in_estab(sid_t s, void *context, void *argA, void *argB);
void in_closewait(sid_t s, void *context, void *argA, void *argB);
void in_timewait(sid_t s, void *context, void *argA, void *argB);

#endif
