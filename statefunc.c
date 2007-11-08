#include <assert.h>
#include <stdint.h>

#include "van_driver.h"
#include "statefunc.h"
#include "tcp.h"

int send_packet_with_flags(sid_t prev, sid_t next, void* context, void *arg, void *tran_arg) {
	tcp_socket_t *sock = (tcp_socket_t*)context;
	assert(sock);
	assert(arg);
	uint8_t flags = *(uint8_t*)arg;

	nlog(MSG_LOG,"state:SPWF", "in send_packet_with_flags, with socket %d", sock->fd);
	nlog(MSG_LOG,"state:SPWF", "flags are: %p", flags);

	char *packet;

	int packet_size = build_tcp_packet(NULL, 0, sock->local_port, sock->remote_port ,
			sock->seq_num, /*ack*/ 0, flags,SEND_WINDOW_SIZE, &packet);

	return 0; // FIXME XXX TODO

}
