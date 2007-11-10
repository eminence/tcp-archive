#ifndef __TCP_H_
#define __TCP_H_

#include <pthread.h>

#include "tcpstate.h"
#include "van_driver.h"

/* Forward declaration */
struct tcp_machine__;
struct ip_node__;

typedef struct tcp_socket__ {
	struct tcp_machine__ *machine;
	unsigned int fd;
	uint32_t seq_num;
	uint32_t ack_num;

	/* Socket identifiers. */
	struct ip_node__ *local_node;
	unsigned short local_port;
	int remote_node;
	unsigned short remote_port;

	pthread_cond_t cond;
	pthread_mutex_t lock;
	int cond_status;

} tcp_socket_t;

void v_tcp_init();
void v_tcp_destroy();
int v_socket();
int v_bind(int socket, int node, unsigned short port);
int v_listen(int socket, int backlog /* optional */);
int v_connect(int socket, int node, unsigned short port);
int v_accept(int socket, int *node);
int v_read(int socket, unsigned char *buf, int nbyte);
int v_write(int socket, const unsigned char *buf, int nbyte);
int v_close(int socket);

int build_tcp_packet(char *data, int data_size, 
		uint16_t source_port, uint16_t dest_port,
		uint32_t seq_num, uint32_t ack_num,
		uint8_t flags, uint16_t window, char **header);


int tcp_sendto(tcp_socket_t* sock, char * data_buf, int bufsize, uint8_t flags);

#endif
