#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "notify.h"

void notify(tcp_socket_t *sock, int status) {
	nlog(MSG_LOG,"notify", "attempting to get socket lock to set sock %d status %p", sock->fd, status);
	pthread_mutex_lock(&sock->lock);
	sock->cond_status = status;
	nlog(MSG_LOG,"notify", "calling pthread_cond_signal (for socket %d)", sock->fd);
	pthread_cond_signal(&sock->cond);
	pthread_mutex_unlock(&sock->lock);
}

int has_status(int want, int have) {
	return !!(want & have);
}

int wait_for_event(tcp_socket_t *sock, int status_bits) {	
	pthread_mutex_lock(&sock->lock);
  
  nlog(MSG_LOG, "wait_for_event", "sock %d want: %#x, have: %#x", sock->fd, status_bits, sock->cond_status);

	while (!(has_status(status_bits, sock->cond_status))) {
		nlog(MSG_LOG,"wait_for_event", "sleeping on cond var");
		pthread_cond_wait(&sock->cond, &sock->lock);
		nlog(MSG_LOG,"wait_for_event", "cond var woke me up");

	}

	int to_return = sock->cond_status;
	/* reset status. */
	sock->cond_status = 0;

	pthread_mutex_unlock(&sock->lock);

	return to_return;
}

