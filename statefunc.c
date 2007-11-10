#include <assert.h>
#include <stdint.h>

#include "van_driver.h"
#include "statefunc.h"
#include "tcp.h"
#include "socktable.h"


int send_packet_with_flags(sid_t prev, sid_t next, void* context, void *arg, void *tran_arg) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);
	assert(arg);
	uint8_t flags = *(uint8_t*)arg;

	nlog(MSG_LOG,"state:SPWF", "in send_packet_with_flags, with socket %d", sock->fd);
	nlog(MSG_LOG,"state:SPWF", "flags are: %p", flags);

	tcp_sendto(sock, NULL, 0, flags);

	return 0; // FIXME XXX TODO
}


int do_listen(sid_t prev, sid_t next, void* context, void *arg, void *tran_arg) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);

	ip_node_t *node = (ip_node_t*)tran_arg;
	assert(node);

	socktable_put(node->tuple_table, sock, HALF_SOCKET);
	nlog(MSG_LOG,"do_listen", "just called socktable_put.  i hope it worked");
	socktable_dump(node->tuple_table, FULL_SOCKET);
	socktable_dump(node->tuple_table, HALF_SOCKET);

	return 0;
}

void fail_in_estab() {


}

int has_status(int bit, int bits) {
	return (bits & bit) == bit;
}

int wait_for_event(tcp_socket_t *sock, int status_bits) {	


	pthread_mutex_lock(&sock->lock);

	while (!(has_status(status_bits, sock->cond_status))) {
		pthread_cond_wait(&sock->cond, &sock->lock);

	}
	pthread_mutex_unlock(&sock->lock);
	return sock->cond_status;
}

void notify(tcp_socket_t *sock, int status) {
	
	pthread_mutex_lock(&sock->lock);
	sock->cond_status = status;
	pthread_cond_signal(&sock->cond);
	pthread_mutex_unlock(&sock->lock);

}

void in_estab(sid_t s, void *context, void *argA, void *argB) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);


	nlog(MSG_LOG,"state:in_estab", "We're not in the established state.  Attempted to notify the user");

	notify(sock,TCP_OK);

	return;
}
