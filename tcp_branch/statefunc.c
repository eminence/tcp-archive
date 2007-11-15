#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "van_driver.h"
#include "statefunc.h"
#include "tcp.h"
#include "socktable.h"
#include "fancy_display.h"
#include "notify.h"
#include "cbuffer.h"

int do_close(__attribute__((unused)) sid_t prev, __attribute__((unused)) sid_t next, void* context, void* close_type, __attribute__((unused)) void* argB) {
	tcp_socket_t* sock = (tcp_socket_t*)context;


	switch(*(int*)close_type) {
		case CLOSE_ERROR:
			nlog(MSG_LOG, "do_close", "close_type=CLOSE_ERROR");
			notify(sock, TCP_ERROR);
			break;
		
		case CLOSE_OK:
			nlog(MSG_LOG, "do_close", "close_type=CLOSE_OK");
			notify(sock, TCP_OK);
			break;

		case CLOSE_EOF:
			nlog(MSG_LOG, "do_close", "close_type=CLOSE_EOF");
			queue_eof(sock);
			break;

		case CLOSE_NIL:
			nlog(MSG_LOG, "do_close", "close_type=CLOSE_NIL. wtf?");
			/* nothing */
			break;
	}

	return 0;
}

int do_connect(__attribute__((unused)) sid_t prev, __attribute__((unused)) sid_t next, void* context, void* rflags, void* packet) {
  tcp_socket_t* sock = (tcp_socket_t*)context;
  uint32_t incoming_seq_num = get_seqnum(ip_to_tcp((char*)packet)) ;
  
  nlog(MSG_LOG, "do_connect", "Setting client's recv_next to %d, recv_read to one less", incoming_seq_num + sock->recv_next);

	/* This works because we have technically already moved our ack pointer forward by one before
	 * hitting the state machine (it's init to 0 and we add 1, hence 1). We do this because, later,
	 * we'll want to have the latest ack pointer if we should send a packet while in the state machine. */

  sock->recv_next += incoming_seq_num;
  sock->recv_ack += incoming_seq_num;
  sock->recv_read = incoming_seq_num; // we bump past the initial syn BEFORE this assignment; thus, we must also bump here!
  
  /* XXX Let's put this hack on hold. */
  //sock->recv_read = incoming_seq_num +1; // we bump past the initial syn BEFORE this assignment; thus, we must also bump here!

  //sock->send_window_size = sock->recv_read + sock->recv_window_size - sock->recv_next;

  /* manually bump up pointers, to represent the SYN that we sent, and that got ACKd */
  //sock->send_next++;
  //sock->send_written++; /* XXX already done in queue_up_flags called by send_packet_with_flags */
  //sock->send_una++;

  return send_packet_with_flags((tcp_socket_t*)context, *((uint8_t*)rflags), 0);
}

int do_send_flags(__attribute__((unused)) sid_t prev,__attribute__((unused))  sid_t next, void* context, void *arg, __attribute__((unused)) void *tran_arg) {
  return send_packet_with_flags((tcp_socket_t*)context, *((uint8_t*)arg), 0);
}

int do_listen(__attribute__((unused)) sid_t prev,__attribute__((unused))  sid_t next, void* context,__attribute__((unused))  void *arg, void *tran_arg) {
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


void fail_with_reset(__attribute__((unused)) sid_t id, void* context, void* packet) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);

	nlog(MSG_LOG, "fail_with_reset", "failing socket %d.  sending a RST and moving back to the closed state ");

	// TODO: send reset with correct sequence number from packet (args)
	send_dumb_packet(sock, (char*)packet, TCP_FLAG_RST);
	sock->last_packet = 0;

	return; 
}

void in_timewait(sid_t s, void *context, __attribute__((unused)) void *argA, __attribute__((unused)) void *argB) {
	tcp_socket_t *sock = (tcp_socket_t*)context;

	/* set a flag in sock, so this socket will move to ST_CLOSED*/
	assert(s = ST_TIME_WAIT);
	sock->time_wait_time = time(NULL);


}
void in_estab(sid_t s, void *context, __attribute__((unused)) void *argA, __attribute__((unused)) void *argB) {
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
void in_closewait(sid_t s, void *context, __attribute__((unused)) void *argA, __attribute__((unused)) void *argB) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	sid_t prev_state = tcpm_prevstate(sock->machine);
	assert(sock);

	if(s != prev_state) {
	  queue_eof((tcp_socket_t*)context);
	  nlog(MSG_LOG, "in_closewait", "remote host closed output!");
	}
}

/* Called when we enter close state (but not the first time through... yet. */
void in_closed(__attribute__((unused)) sid_t s, void *context, __attribute__((unused)) void *argA, void *reason) {
	tcp_socket_t* sock = (tcp_socket_t*)context;

	switch((int)reason) {
		case RESTART_INIT:
			// initialize socket
			pthread_cond_init(&sock->cond, NULL);
			pthread_mutex_init(&sock->lock, NULL);
			nlog(MSG_XXX, "in_closed (lock)", "init mutexes! for sock %d", sock->fd);
			pthread_mutex_init(&sock->protect, NULL);
		 	sock->r_buf = 0;
		 	sock->s_buf = 0;
			sock->cond_status = 0;

		case RESTART_OK:
		case RESTART_ABORT:
			// Re-initialize socket
			sock->last_packet = 0;

			sock->local_port = rand()%65535;
			sock->remote_port = 0;
			sock->remote_node = 0;
			sock->seq_num = 0;
			sock->ack_num = 0;
			sock->can_handshake = 0;
			sock->parent = NULL;
			sock->last_packet = 0;
			sock->send_window_size = SEND_WINDOW_SIZE;
			sock->recv_window_size = SEND_WINDOW_SIZE;
			sock->time_wait_time = 0;
			sock->ufunc_timeout = 0;

			sock->send_una = 0;
			sock->send_next = 0;
			sock->send_written = 0;
			sock->remote_flow_window = sock->send_next + SEND_WINDOW_SIZE; /* a reasonable default? je pense que oui */

			sock->recv_ack = 0;
			sock->recv_next = 0;
			sock->recv_read = 0;

			if(sock->r_buf)
				cbuf_destroy(sock->r_buf);

			if(sock->s_buf)
				cbuf_destroy(sock->s_buf);

		 	sock->r_buf = cbuf_new(SEND_WINDOW_SIZE*2  + 2);
		 	sock->s_buf = cbuf_new(SEND_WINDOW_SIZE*2  + 2);

		 	nlog(MSG_LOG, "in_closed", "state machine entered Closed state: %s", (int)reason == RESTART_INIT ? "Init" : 
																										(int)reason == RESTART_OK ? "OK" : "Abort");
			

			/* If error, send signal */
			if((int)reason == RESTART_ABORT) {
			  notify(sock, TCP_ERROR); // cause associated sock call to fail
			}

			break;

			default:
				assert(0 && "Invalid reason.");
	}
	
	// TODO fill this up (allow this function to initialize sock by calling on new machine? and free data maybe? or something? */
}
