#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <checksum.h>
#include <pthread.h>

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

	nlog(MSG_LOG,"tcp_sendto", "now have a packet of size %d ready to be sent to dest_port %d", 
			packet_size, sock->remote_port);


	uint16_t dport = get_destport(packet);
	nlog(MSG_LOG,"tcp_sendto", "from build_tcp_packet, dport=%d (should be %d)", dport, sock->remote_port);

	// put our tcp packet in an IP packet. woot!
	//int ip_packet_size = buildPacket(sock->local_node, packet, packet_size, sock->remote_node, &ippacket, PROTO_TCP);

	int retval = van_driver_sendto(sock->local_node, packet, packet_size, sock->remote_node, PROTO_TCP);

	free(packet);

	if (retval == -1) {
		nlog(MSG_ERROR,"tcp_sendto", "van_driver_sendto returned -1! A tcp packet just got lost!");
		return -1;
	}

  /* Increase seq_num by buffer size; add 1 for each special flag. */
  sock->seq_num += bufsize + !!(flags & TCP_FLAG_SYN)
                           + !!(flags & TCP_FLAG_FIN)
                           + !!(flags & TCP_FLAG_RST);

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

	int total_packet_length = data_size + TCP_HEADER_SIZE; // fixed header size of 20 bytes
	
	*header = malloc(total_packet_length);
	memset(*header, 0, total_packet_length); // zero out everything

  nlog(MSG_LOG, "build_tcp_packet", "src_port = %d, dest_port = %d, seq_num = %d, ack_num = %d, flags = %s%s%s%s , window = %d, len = %d",
    source_port, dest_port, seq_num, ack_num, flags & TCP_FLAG_SYN ? " SYN" : "", flags & TCP_FLAG_ACK ? " ACK" : "", flags & TCP_FLAG_RST ? " RST" : "", flags & TCP_FLAG_FIN ? " FIN" : "", window, data_size);

	set_srcport(*header, source_port);
	set_destport(*header, dest_port);
	set_seqnum(*header, seq_num);
	set_acknum(*header, ack_num);

  assert(get_seqnum(*header) == seq_num);
  assert(get_acknum(*header) == ack_num);

	set_flags(*header, flags);
	set_window(*header, window);

	uint16_t dest_port2 = get_destport(*header);
	nlog(MSG_LOG, "build_tcp_packet", "dest_port readback: %d (should be %d)", dest_port2, dest_port);

	// memcpy the data into the packet, if there is any
	if (data) memcpy(*header+TCP_HEADER_SIZE,data,data_size);

	// TODO set checksum

	return data_size + TCP_HEADER_SIZE;
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

  nlog(MSG_LOG, "v_tcp_init", "Creating tuple table.");

	node->tuple_table = malloc(sizeof(socktable_t));
	socktable_init(node->tuple_table);
}

int sys_socket(int clone) {
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

	sock->machine = tcpm_new(sock, clone); 

	sock->fd = s;
	sock->local_port = rand()%65535;
	sock->local_node = this_node;
	sock->can_handshake = 0;
	sock->parent = NULL;
  sock->cond_status = 0;

	tcp_table_new(this_node, s);	

	pthread_cond_init(&sock->cond, NULL);
	pthread_mutex_init(&sock->lock, NULL);

	return s;
}

/* returns a new unbound socket.
 * on failure, returns a negative value */
int v_socket() {
  return sys_socket(0);
}

/* binds a socket to a port
 * returns 0 on success or negative number on failure */
int v_bind(int socket, int node, uint16_t port) {
	tcp_socket_t *sock = get_socket_from_int(socket);
  uint16_t old_port;
  
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

  sock->seq_num = 1000;
  
	if (tcpm_event(sock->machine, ON_PASSIVE_OPEN, this_node, NULL)) {
		nlog(MSG_ERROR,"socket:listen", "Uhh. error.  Couldn't transition states.  noob");
		return -1;
	}

	return 0;
}


/* connects a socket to an address
 * returns 0 on success of a negative number on failure */
int v_connect(int socket, int node, uint16_t port) {
	tcp_socket_t *sock = get_socket_from_int(socket);

	// TODO make sure we can call connect
	// TODO transition into connection state

	sock->remote_node = node;
	sock->remote_port = port;

	sock->seq_num = 100; /* an arbitrary inital seq number. TODO make this random */ 
	sock->ack_num = 0;
	sock->can_handshake = 1;

  /* Update tuple table with FULL socket. */
  socktable_put(this_node->tuple_table, sock, FULL_SOCKET);

	assert(sock->machine);
	if (tcpm_event(sock->machine, ON_ACTIVE_OPEN, NULL, NULL)) {
		nlog(MSG_ERROR,"socket:connect","Uhh. error. Couldn't transition states.  noob");
		return -1;
	}

	int status = wait_for_event(sock, TCP_OK | TCP_CONNECT_FAILED);

	if (status == TCP_OK) return 0;
	else { return -1; }
}

/* accept a requested connection
 * returns new socket handle on success or negative number on failure
 * NOTE: this function will block */
int v_accept(int socket) {
	tcp_socket_t *sock = get_socket_from_int(socket);

	// TODO make sure we can call accept

	sock->can_handshake = 1;
	int status = wait_for_event(sock, TCP_OK | TCP_CONNECT_FAILED);
	sock->can_handshake = 0;

	//tcp_table_new(this_node, sock->new_fd);

  if(status == TCP_OK) {
  	return sock->new_fd;
  } else {
    return -1;
  }

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
