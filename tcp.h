#ifndef __TCP_H_
#define __TCP_H_

#include <pthread.h>

#include "tcpstate.h"
#include "van_driver.h"
#include "cbuffer.h"

#define LOCALBUFSIZE 4096

/* Forward declaration */
struct tcp_machine__;
struct ip_node__;

typedef struct tcp_socket__ {
	struct tcp_machine__ *machine;
	unsigned char can_handshake; /* set by v_accept() */
	unsigned int fd;
	uint32_t seq_num; /* this is the value we will use for our next output packet */
	uint32_t ack_num; /* this is the value we will use for our next output packet */

	uint32_t last_ack; /* this is the number of the last ACKNUM we've received */

	/* Socket identifiers. */
	struct ip_node__ *local_node;
	uint16_t local_port;
	int remote_node;
	uint16_t remote_port;

	pthread_cond_t cond;
	pthread_mutex_t lock;
	int cond_status;

	int new_fd;
	struct tcp_socket__ *parent;

	cbuf_t *r_buf;
	cbuf_t *s_buf;

	int send_una; /* data before this is acked.  after this is sent, but unacked */
	int send_next; /* data before this is send, but unakced.   after this is not sent, but OK to send */
	int send_written; /* points to the next location in the buffer we can write incoming data */

	int remote_flow_window; /* the point were we cant send any more due to the remote side flowing off */

	int recv_next; /* data before this already recvd and ackd.  after this, not yet, but ok to receive*/
	int recv_read; /* points to the next data in the buffer to be returned via a v_read() call.  any data before this can be overwritten */

	int recv_window_size;
	int send_window_size; /* our window size we advertize in outgoing packets */

	time_t last_packet; /* set this to time(NULL) when you're expecting a timely reply. a clocktick thread will alert someone of something when something happens */
	
} tcp_socket_t;

void v_tcp_init();
void v_tcp_destroy();
int v_socket();
int v_bind(int socket, int node, uint16_t port);
int v_listen(int socket, int backlog /* optional */);
int v_connect(int socket, int node, uint16_t port);
int v_accept(int socket);
int v_read(int socket, unsigned char *buf, int nbyte);
int v_write(int socket, const unsigned char *buf, int nbyte);
int v_close(int socket);

int sys_socket(int clone);

int build_tcp_packet(char *data, int data_size, 
		uint16_t source_port, uint16_t dest_port,
		uint32_t seq_num, uint32_t ack_num,
		uint8_t flags, uint16_t window, char **header);


int tcp_sendto(tcp_socket_t* sock, char * data_buf, int bufsize, uint8_t flags);
tcp_socket_t *get_socket_from_int(int s);
uint16_t calculate_tcp_checksum(char* packet);
int send_packet_with_flags(tcp_socket_t* sock, uint8_t flags, int ack_size); /* ack_size unused for fin/syn */

#endif
