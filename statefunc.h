#ifndef __STATEFUNC_H_
#define __STATEFUNC_H_

#include "tcp.h"
#include "state.h"

#define TCP_OK 0x00
#define TCP_CONNECT_FAILED 0x01

struct tcp_socket__;

int send_packet_with_flags (sid_t prev, sid_t next, void* context, void *arg, void *tran_arg);
int do_listen(sid_t prev, sid_t next, void* context, void *arg, void *tran_arg);
int wait_for_event(struct tcp_socket__ *sock, int status_bits);
void in_estab(sid_t s, void *context, void *argA, void *argB);

#endif
