#ifndef __TCP_H_
#define __TCP_H_

typedef struct {
	machine_t *machine;
	short local_port;
	short remote_port;
	int remote_node;

} tcp_socket_t;

int v_socket();
int v_bind(int socket, int node, short port);
int v_listen(int socket, int backlog /* optional */);
int v_connect(int socket, int node, short port);
int v_accept(int socket, int *node);
int v_read(int socket, unsigned char *buf, int nbyte);
int v_write(int socket, const unsigned char *buf, int nbyte);
int v_close(int socket);

#endif
