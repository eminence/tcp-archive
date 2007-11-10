#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <checksum.h>

#include "tcp.h"
#include "van_driver.h"
#include "fancy_display.h"
#include "state.h"
#include "tcpstate.h"
#include "socktable.h"
#include "ippacket.h"

static ip_node_t *this_node;

tcp_socket_t *get_socket_from_int(int s) {
	assert(s >= 0);
	assert(s < MAXSOCKETS);
	tcp_socket_t *sock = this_node->socket_table[s];
	assert(sock);

	return sock;
}

/* master tcp sender function
 */
int tcp_sendto(tcp_socket_t* sock, char * data_buf, int bufsize, uint8_t flags) {
	assert(sock);

	char *packet;

	int packet_size = build_tcp_packet(data_buf, bufsize, sock->local_port, sock->remote_port ,
			sock->seq_num, /*ack*/ sock->ack_num, flags, SEND_WINDOW_SIZE, &packet);

	nlog(MSG_LOG,"tcp_sendto", "now have a packet of size %d ready to be sent", packet_size);

	// put our tcp packet in an IP packet. woot!
	//int ip_packet_size = buildPacket(sock->local_node, packet, packet_size, sock->remote_node, &ippacket, PROTO_TCP);

	int retval = van_driver_sendto(sock->local_node, packet, packet_size, sock->remote_node, PROTO_TCP);

	free(packet);

	if (retval == -1) {
		nlog(MSG_ERROR,"tcp_sendto", "van_driver_sendto returned -1! A tcp packet just got lost!");
		return -1;
	}

	if ((flags & TCP_FLAG_SYN) == TCP_FLAG_SYN) {
		sock->seq_num++; /* increase seq num by one */
	} else if ((flags & TCP_FLAG_FIN) == TCP_FLAG_FIN) {
		sock->seq_num++;
	} else {
		assert (bufsize > 0); /* TODO double check that this assertion is valid */
		sock->seq_num += bufsize;
	}

	return retval;
}

/*
 * mallocs some memory, and builds a complete tcp packet
 * (suitable to sticking directly into an IP packet)
 *
 * returns the size of the new packet
 */
int build_tcp_packet(char *data, int data_size, 
		uint16_t source_port, uint16_t dest_port,
		uint32_t seq_num, uint32_t ack_num,
		uint8_t flags, uint16_t window, char **header) {

	int total_packet_length = data_size + 20; // fixed header size of 20 bytes
	
	*header = malloc(total_packet_length);
	memset(*header, 0, total_packet_length); // zero out everything

	set_srcport(*header, source_port);
	set_destport(*header, dest_port);
	set_seqnum(*header, seq_num);
	set_acknum(*header, ack_num);
	set_flags(*header, flags);
	set_window(*header, window);

	// memcpy the data into the packet, if there is any
	if (data) memcpy(*header+20,data,data_size);

	// TODO set checksum

	return 0;
}

/* destroy global tcp structures.
 */
void v_tcp_destroy(ip_node_t *node) {
  socktable_destroy(node->tuple_table);
}

/* initialize various tcp structures
 */
void v_tcp_init(ip_node_t *node) {

	int x;
	this_node = node;
	for (x = 0; x < MAXSOCKETS; x++) {
		this_node->socket_table[x] = NULL;
	}


	node->tuple_table = malloc(sizeof(socktable_t));
	socktable_init(node->tuple_table);
}

/* returns a new unbound socket.
 * on failure, returns a negative value */
int v_socket() {
	int s = -1;
	tcp_socket_t *sock = NULL;

	// find the first unused socket
	int x;
	for (x = 0; x < MAXSOCKETS; x++){
		if (this_node->socket_table[x] == NULL) { s = x; break; }
	}

	if (s == -1) {
		// we cant find a free socket, return error
		return -1;
	}

	sock = malloc(sizeof(tcp_socket_t) );
	assert(sock);
	this_node->socket_table[s] = sock;

	sock->machine = tcpm_new(sock); 

	sock->fd = s;
	sock->local_port = rand()%65535;
	sock->local_node = this_node;

	return s;
}

/* binds a socket to a port
 * returns 0 on success or negative number on failure */
int v_bind(int socket, int node, short port) {
	tcp_socket_t *sock = get_socket_from_int(socket);
  short old_port;
  
	assert(sock->machine);
	if (tcpm_state(sock->machine) == ST_CLOSED) {
		sock->local_port = port;

	} else {
		nlog(MSG_ERROR,"v_bind","Someone called v_bind, but I'm not in the CLOSED state!");
		return -1;
	}

	return 0;
}

/* moves a socket into listen state
 * returns 0 on success or negative number on failure */
int v_listen(int socket, int backlog /* optional */) {
	tcp_socket_t *sock = get_socket_from_int(socket);

	assert(sock->machine);
	if (tcpm_event(sock->machine, ON_PASSIVE_OPEN, NULL, NULL)) {
		nlog(MSG_ERROR,"socket:listen", "Uhh. error.  Couldn't transition states.  noob");
		return -1;

	}

	return 0;
}


/* connects a socket to an address
 * returns 0 on success of a negative number on failure */
int v_connect(int socket, int node, short port) {
	tcp_socket_t *sock = get_socket_from_int(socket);

	// TODO make sure we can call connect
	// TODO transition into connection state

	sock->remote_node = node;
	sock->remote_port = port;

	sock->seq_num = 100; /* an arbitrary inital seq number. TODO make this random */ 
	sock->ack_num = 0;

  /* Update tuple table with FULL socket. */
  socktable_put(this_node->tuple_table, sock, FULL_SOCKET);

	assert(sock->machine);
	if (tcpm_event(sock->machine, ON_ACTIVE_OPEN, NULL, NULL)) {
		nlog(MSG_ERROR,"socket:connect","Uhh. error. Couldn't transition states.  noob");
		return -1;
	}

	// TODO wait until we get into the ESTAB state 
	// OR return -1 if we never get there

	return 0;
}

/* accept a requested connection
 * returns new socket handle on success or negative number on failure
 * NOTE: this function will block */
int v_accept(int socket, int *node) {
	tcp_socket_t *sock = get_socket_from_int(socket);

	// TODO make sure we can call accept
	// TODO block and wait for a new incoming connection


	return 0;
}


/* read on an open socket
 * return num types read or negative number on failure or 0 on EOF */
int v_read(int socket, unsigned char *buf, int nbyte) {
	tcp_socket_t *sock = get_socket_from_int(socket);
	
	// TODO make sure we're in the established state;
	// TODO read data

	return 0;
}

/* write on an open socket
 * retursn num bytes written or negative number on failure */
int v_write(int socket, const unsigned char *buf, int nbyte) {
	tcp_socket_t *sock = get_socket_from_int(socket);

	return 0;
}

/* close an open socket
 * returns 0 on success, or negative number on failure */
int v_close(int socket) {

	return 0;
}
