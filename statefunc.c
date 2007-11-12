#include <assert.h>
#include <stdint.h>

#include "van_driver.h"
#include "statefunc.h"
#include "tcp.h"
#include "socktable.h"
#include "fancy_display.h"

int do_connect(sid_t prev, sid_t next, void* context, void* rflags, void* packet) {
  tcp_socket_t* sock = (tcp_socket_t*)context;
  uint32_t incoming_seq_num = get_seqnum(ip_to_tcp((char*)packet)) ;
    
  nlog(MSG_LOG, "do_connect", "Setting client's recv_next and recv_read to %d", incoming_seq_num + 1);

  sock->recv_next = incoming_seq_num + 1;
  sock->recv_read = incoming_seq_num + 1;

  return send_packet_with_flags(prev, next, context, rflags, NULL);
}

int send_packet_with_flags(sid_t prev, sid_t next, void* context, void *arg, void *tran_arg) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);
	assert(arg);
	uint8_t flags = *(uint8_t*)arg;

	nlog(MSG_LOG,"state:SPWF", "in send_packet_with_flags, with socket %d flags are %p", sock->fd, flags);

  /* Use special case sequence number increase. */
	tcp_sendto(sock, NULL, 0, flags);


	return 0; // FIXME XXX TODO
}

int do_listen(sid_t prev, sid_t next, void* context, void *arg, void *tran_arg) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);

	ip_node_t *node = (ip_node_t*)tran_arg;
	assert(node);

	socktable_put(node->tuple_table, sock, HALF_SOCKET);
	//nlog(MSG_LOG,"do_listen", "just called socktable_put.  i hope it worked");
	//socktable_dump(node->tuple_table, FULL_SOCKET);
	//socktable_dump(node->tuple_table, HALF_SOCKET);

	return 0;
}


void fail_with_reset(sid_t id, void* context, void* args) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);

	nlog(MSG_LOG, "fail_with_reset", "failing socket %d.  sending a RST and moving back to the closed state");

	tcp_sendto(sock, NULL, 0, TCP_FLAG_RST);

	return; 
}

int has_status(int want, int have) {
	return !!(want & have);
}

int wait_for_event(tcp_socket_t *sock, int status_bits) {	
	pthread_mutex_lock(&sock->lock);
  
  nlog(MSG_LOG, "wait_for_event", "want: %#x, have: %#x", status_bits, sock->cond_status);

	while (!(has_status(status_bits, sock->cond_status))) {
		nlog(MSG_LOG,"wait_for_event", "sleeping on cond var");
		pthread_cond_wait(&sock->cond, &sock->lock);
		nlog(MSG_LOG,"wait_for_event", "cond var woke me up");

	}
	pthread_mutex_unlock(&sock->lock);
	int to_return = sock->cond_status;
	sock->cond_status = 0;

	return to_return;
}

void notify(tcp_socket_t *sock, int status) {

	nlog(MSG_LOG,"notify", "attempting to get socket lock");
	pthread_mutex_lock(&sock->lock);
	sock->cond_status = status;
	nlog(MSG_LOG,"notify", "calling pthread_cond_signal (for socket %d)", sock->fd);
	pthread_cond_signal(&sock->cond);
	pthread_mutex_unlock(&sock->lock);

}

void in_estab(sid_t s, void *context, void *argA, void *argB) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);

	nlog(MSG_LOG,"state:in_estab", "We're now in the established state.  Attempted to notify the user");
  
  /* If argB set, wake up old (parent) socket instead of current socket. */
  if(sock->parent) {
    notify(sock->parent, TCP_OK);
  } else {
  	notify(sock, TCP_OK);
  }

	return;
}
