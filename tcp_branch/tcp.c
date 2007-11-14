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
#include "seq.h"
#include "notify.h"

static ip_node_t *this_node;

/* calculate something that looks like a TCP checksum 
 *
 * loosely copied from http://www.netfor2.com/tcpsum.htm
 * */
uint16_t tcp_sum_calc(char *buf, int len) {
	uint16_t padd = 0;
	uint16_t word16;
	uint32_t sum;
	int i;

	// if size is odd, add padding byte
	if (len%2 == 1) {
		padd = 1;
	}

	sum = 0;

	// make 16 bit words out of every tow adjacent 8 bit words and
	// calculate the sum of all 16 bit words
	for (i = 0; i<len;i=i+2) {

		if ((i=len-1) && (padd)) {
			word16 = ((buf[i]<<8)&0xFF00)+(0&0xff);
		} else {
			word16 = ((buf[i]<<8)&0xFF00)+(buf[i+1]&0xff);
		}
		sum = sum + (uint32_t)word16;
	}

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	sum = ~sum;

	return (uint16_t)sum;

}


tcp_socket_t* get_tmp_socket(uint16_t local_port, int remote_node, uint16_t remote_port, uint16_t send_window_size) {
	tcp_socket_t* sock = malloc(sizeof(tcp_socket_t));

	sock->local_node = this_node;
	sock->local_port = local_port;
	sock->remote_node = remote_node;
	sock->remote_port = remote_port;
	sock->send_window_size = send_window_size;

	return sock;
}

int queue_eof(tcp_socket_t *sock) {
  /* WARNING: increasing ack number. This should be fine because we SHOULD NOT be in a recv state
	* (we just received a FIN). However, if there are bugs later, remember to look at this code.
	*/
  cbuf_put_eof(sock->r_buf, sock->recv_next++);

  return 0;
}

int queue_up_flags(tcp_socket_t *sock, uint8_t flags) {
	nlog(MSG_LOG, "queue_up_flags", "queueing flags for send: %p", flags);

	int start = sock->send_written;
	cbuf_put_flag(sock->s_buf, start, flags);

	/* Bump the written pointer up-- we wrote a byte */
	sock->send_written++;
	// sock->recv_next++; // TODO: this line is bad, I think
	
	return 0;
}

int send_packet_with_flags(tcp_socket_t* sock, uint8_t flags, int ack_len) {
	nlog(MSG_LOG,"spwf", "socket %d, flags are %p", sock->fd, flags);

  /* Send ACK raw; they don't live in seq. num space. */
  if(flags == TCP_FLAG_ACK) {
  	/* Why the heck would we ACK data recvd right before sending data ourselves??? XXX */
	// ackData(sock, ack_len ? ack_len : !!(flags & ~TCP_FLAG_ACK)); // if len == 0, only ack 1 byte if flag not just ACK
    tcp_sendto(sock, NULL, 0, TCP_FLAG_ACK);
  } else {
    queue_up_flags(sock, flags);							// other flag combinations have size 1 byte
  }

	return 0;
}

tcp_socket_t *get_socket_from_int(int s) {
	assert(s >= 0);
	assert(s < MAXSOCKETS);
	tcp_socket_t *sock = this_node->socket_table[s];
	
  /* Learn of unassigned slots by returning NULL. */
  // assert(sock);

	return sock;
}


int send_dumb_packet(tcp_socket_t *sock, char*packet, uint8_t AckOrRST) {

	uint32_t seq = get_seqnum(ip_to_tcp(packet));
	uint32_t ack = get_acknum(ip_to_tcp(packet));
	uint8_t flags = get_flags(ip_to_tcp(packet));

	if (AckOrRST == TCP_FLAG_ACK) 
		tcp_sendto_raw(sock, NULL, 0, TCP_FLAG_ACK, ack, sock->recv_next);
	if (AckOrRST == TCP_FLAG_RST)
		tcp_sendto_raw(sock, NULL, 0, TCP_FLAG_RST, ack, sock->recv_next);


	return 0;

}


int tcp_sendto_raw(tcp_socket_t* sock, char * data_buf, int bufsize, uint8_t flags, uint32_t seq, uint32_t ack) {
	assert(sock);

	char *packet;

	//XXX seqnum is obsolete and meaningless

	//int packet_size = build_tcp_packet(data_buf, bufsize, sock->local_port, sock->remote_port ,
	//		sock->seq_num, /*ack*/ sock->ack_num, flags, sock->send_window_size, &packet);
	int packet_size = build_tcp_packet(data_buf, bufsize, sock->local_port, sock->remote_port ,
			seq, ack, flags, sock->send_window_size, &packet);

	nlog(MSG_LOG,"tcp_sendto", "now have a packet of size %d ready to be sent to dest_port %d", 
			packet_size, sock->remote_port);

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


	/* as long as the packet is not a pure ACK (where pure ACK == only ACK, and no payload), then
	 * we'll be expecting a reply for this packet */
	if (!((flags == TCP_FLAG_ACK) && (bufsize == 0))) {
		if(!sock->last_packet)
			sock->last_packet = time(NULL);
	} else {
		sock->last_packet = 0;
	}

	return retval;
}

/* master tcp sender function
 */
int tcp_sendto(tcp_socket_t* sock, char * data_buf, int bufsize, uint8_t flags) {
	assert(sock);

	char *packet;

	//XXX seqnum is obsolete and meaningless

	//int packet_size = build_tcp_packet(data_buf, bufsize, sock->local_port, sock->remote_port ,
	//		sock->seq_num, /*ack*/ sock->ack_num, flags, sock->send_window_size, &packet);
	int packet_size = build_tcp_packet(data_buf, bufsize, sock->local_port, sock->remote_port ,
			sock->send_next, /*ack*/ sock->recv_next, flags, sock->recv_read + sock->recv_window_size - sock->recv_next, &packet);

	nlog(MSG_LOG,"tcp_sendto", "now have a packet of size %d ready to be sent to dest_port %d", 
			packet_size, sock->remote_port);

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


	/* as long as the packet is not a pure ACK (where pure ACK == only ACK, and no payload), then
	 * we'll be expecting a reply for this packet */
	if (!((flags == TCP_FLAG_ACK) && (bufsize == 0))) {
		if(!sock->last_packet)
			sock->last_packet = time(NULL);
	} else {
		sock->last_packet = 0;

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

	// memcpy the data into the packet, if there is any
	if (data) memcpy(*header+TCP_HEADER_SIZE,data,data_size);

	uint16_t csum;

	set_tcpchecksum(*header, (csum = calculate_tcp_checksum(tcp_to_ip(*header)))); 
	assert(csum == get_tcpchecksum(*header));

	return data_size + TCP_HEADER_SIZE;
}

uint16_t calculate_tcp_checksum(char* packet) {

	set_tcpchecksum(packet, 0);
	return tcp_sum_calc(packet, get_total_len(tcp_to_ip(packet)));
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
	nlog(MSG_LOG, "v_tcp_init", "Creating tuple table: %p", node->tuple_table);
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

	/* see tcp.h for some descriptions of what these are */
	sock->fd = s;
	sock->local_port = rand()%65535;
	sock->remote_port = 0;
	sock->local_node = this_node;
	sock->remote_node = 0;
	sock->seq_num = 0;
	sock->ack_num = 0;
	sock->can_handshake = 0;
	sock->parent = NULL;
	sock->cond_status = 0;
	sock->last_packet = 0;
	sock->send_window_size = SEND_WINDOW_SIZE;
	sock->recv_window_size = SEND_WINDOW_SIZE;
	sock->time_wait_time = 0;

	sock->send_una = 0;
	sock->send_next = 0;
	sock->send_written = 0;
	sock->remote_flow_window = sock->send_next + SEND_WINDOW_SIZE; /* a reasonable default? je pense que oui */

	sock->recv_next = 0;
	sock->recv_read = 0;

	sock->r_buf = cbuf_new(SEND_WINDOW_SIZE*2  + 2);
	sock->s_buf = cbuf_new(SEND_WINDOW_SIZE*2  + 2);
	
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

	sock->send_una = sock->seq_num;
	sock->send_next = sock->seq_num;
	sock->send_written = sock->seq_num;

  /* Update tuple table with FULL socket. */
  socktable_put(this_node->tuple_table, sock, FULL_SOCKET);

	assert(sock->machine);
	if (tcpm_event(sock->machine, ON_ACTIVE_OPEN, NULL, NULL)) {
		nlog(MSG_ERROR,"socket:connect","Uhh. error. Couldn't transition states.  noob");
		return -1;
	}

	int status = wait_for_event(sock, TCP_OK | TCP_ERROR);

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
	int status = wait_for_event(sock, TCP_OK | TCP_ERROR);
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

	/* XXX NOTE! XXX v_read is NON BLOCKING! WOOT! */

	if (nbyte == 0) return 0;

	int amount;

	if ((amount=amountOfDataToRead(sock)) == 0) return 0;
	else {

		memset(buf, 0, nbyte); /* memset... always a good decision */
		int retval = getDataFromBuffer(sock, buf, amount);
		return retval;

	}

	// TODO make sure we're in the established state;

}

/* write on an open socket
 * retursn num bytes written or negative number on failure */
int v_write(int socket, const unsigned char *buf, int nbytes) {
	tcp_socket_t *sock = get_socket_from_int(socket);

	int can_send = getAmountAbleToAccept(sock);
	int will_send = MIN(can_send, nbytes);

	nlog(MSG_LOG,"v_write", "have room to accept %d bytes of data.  Will send %d bytes", can_send, will_send);

	copyDataFromUser(sock, buf, will_send);

	can_send = getAmountAbleToAccept(sock);
	nlog(MSG_LOG,"v_write", "now, postwrite, can accept %d of data", can_send);

	return will_send;
}

/* close an open socket
 * returns 0 on success, or negative number on failure */
int v_close(int socket) {
	tcp_socket_t *sock = get_socket_from_int(socket);

  /* Update tuple table with FULL socket. */
  socktable_put(this_node->tuple_table, sock, FULL_SOCKET);

	assert(sock->machine);
	if (tcpm_event(sock->machine, ON_CLOSE, NULL, NULL)) {
		nlog(MSG_ERROR,"socket:close","Couldn't close socket.  noob");
		return -1;
	}

	int status = wait_for_event(sock, TCP_OK);

	if (status == TCP_OK) return 0;
	else { return -1; }
  
	return 0;
}
