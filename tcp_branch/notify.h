#ifndef __NOTIFY_H_
#define __NOTIFY_H_

#include "tcp.h"
#include "state.h"

#define TCP_OK              0x01
#define TCP_ERROR			  	 0x02
#define TCP_TIMEOUT			 0x04

struct tcp_socket__;

int wait_for_event(struct tcp_socket__ *sock, int status_bits);
void notify(struct tcp_socket__ *sock, int status_bits);

#endif
