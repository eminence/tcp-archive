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
  
  nlog(MSG_LOG, "do_connect", "Setting client's recv_next to %d, recv_read to one less", incoming_seq_num + sock->recv_next);

	/* This works because we have technically already moved our ack pointer forward by one before
	 * hitting the state machine (it's init to 0 and we add 1, hence 1). We do this because, later,
	 * we'll want to have the latest ack pointer if we should send a packet while in the state machine. */

  sock->recv_next += incoming_seq_num;
  sock->recv_read = incoming_seq_num; //**** (recv_read is the next byte to read so it should stay the same)

  /* manually bump up pointers, to represent the SYN that we sent, and that got ACKd */
  //sock->send_next++;
  //sock->send_written++; /* XXX already done in queue_up_flags called by send_packet_with_flags */
  //sock->send_una++;

  return send_packet_with_flags((tcp_socket_t*)context, *((uint8_t*)rflags), 0);
}

int do_send_flags(sid_t prev, sid_t next, void* context, void *arg, void *tran_arg) {
  return send_packet_with_flags((tcp_socket_t*)context, *((uint8_t*)arg), 0);
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
