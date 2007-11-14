#include <assert.h>
#include <stdint.h>

#include "van_driver.h"
#include "statefunc.h"
#include "tcp.h"
#include "socktable.h"
#include "fancy_display.h"
#include "notify.h"
#include "cbuffer.h"

int do_close(sid_t prev, sid_t next, void* context, void* close_type, void* argB) {
	tcp_socket_t* sock = (tcp_socket_t*)context;


	switch(*(int*)close_type) {
		case CLOSE_ERROR:
			nlog(MSG_XXX, "do_close", "close_type=CLOSE_ERROR");
			notify(sock, TCP_ERROR);
			break;
		
		case CLOSE_OK:
			nlog(MSG_XXX, "do_close", "close_type=CLOSE_OK");
			notify(sock, TCP_OK);
			break;

		case CLOSE_EOF:
			nlog(MSG_XXX, "do_close", "close_type=CLOSE_EOF");
			queue_eof(sock);
			break;

		case CLOSE_NIL:
			nlog(MSG_XXX, "do_close", "close_type=CLOSE_NIL. wtf?");
			/* nothing */
			break;
	}

	return 0;
}

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


void fail_with_reset(sid_t id, void* context, void* packet) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);

	nlog(MSG_LOG, "fail_with_reset", "failing socket %d.  sending a RST and moving back to the closed state ");

	// TODO: send reset with correct sequence number from packet (args)
	send_dumb_packet(sock, (char*)packet, TCP_FLAG_RST);
	sock->last_packet = 0;

	return; 
}

void in_timewait(sid_t s, void *context, void *argA, void *argB) {
	tcp_socket_t *sock = (tcp_socket_t*)context;

	/* set a flag in sock, so this socket will move to ST_CLOSED*/
	assert(s = ST_TIME_WAIT);
	sock->time_wait_time = time(NULL);


}
void in_estab(sid_t s, void *context, void *argA, void *argB) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	sid_t prev_state = tcpm_prevstate(sock->machine);
	assert(sock);

	/* TODO
	 * only notify the user if we've only just entered estab
	 * (i.e. don't notify the user if we previously were already in estab */

	if (prev_state != s) {

		nlog(MSG_LOG,"state:in_estab", "We're now in the established state.  Attempted to notify the user");

		/* If argB set, wake up old (parent) socket instead of current socket. */
		if(sock->parent) {
			notify(sock->parent, TCP_OK);
		} else {
			notify(sock, TCP_OK);
		}

	}

	return;
}

/* Queue an EOF in recv buffer; we should NOT receive any new packets. */
void in_closewait(sid_t s, void *context, void *argA, void *argB) {
	queue_eof(context);
}

/* Called when we enter close state (but not the first time through... yet. */
void in_closed(sid_t s, void *context, void *argA, void *argB) {
	tcp_socket_t* sock = (tcp_socket_t*)context;

	/* Reinit sock. */
	sock->last_packet = 0;
	// TODO fill this up (allow this function to initialize sock by calling on new machine? and free data maybe? or something? */
}
