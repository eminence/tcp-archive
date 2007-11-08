#ifndef __TCP_H_
#define __TCP_H_

#include "state.h"

#define MAXSOCKETS 256

#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_ACK 0x10

/* uint16_t */
#define get_srcport(p) 		(*((uint16_t*)((p))))
#define set_srcport(p,v)	do {uint16_t _tmp=(v); memcpy((p), &_tmp, 2);} while(0)

/* uint16_t */
#define get_destport(p) 		(*((uint16_t*)((p)+2)))
#define set_destport(p,v)		do {uint16_t _tmp=(v); memcpy((p)+2, &_tmp, 2);} while(0)

/* uint32_t */
#define get_seqnum(p)			(*((uint32_t*)((p)+4)))
#define set_seqnum(p,v)			do {uint32_t _tmp=(v); memcpy((p)+4, &_tmp, 4);} while(0)

/* uint32_t */
#define get_acknum(p)			(*((uint32_t*)((p)+8)))
#define set_acknum(p,v)			do {uint32_t _tmp=(v); memcpy((p)+8, &_tmp, 4);} while(0)

/* uint16_t */
#define get_window(p)			(*((uint16_t*)((p)+14)))
#define set_window(p,v)			do {uint16_t _tmp=(v); memcpy((p)+14, &_tmp, 2); } while(0)

/* uint16_t */
#define get_tcpchecksum(p)			(*((uint16_t*)((p)+16)))
#define set_tcpchecksum(p,v)		do {uint16_t _tmp=(v); memcpy((p)+16, &_tmp, 2); } while(0)

#define set_flags(p,v)			((p[13] = ((uint8_t)(v))))

/* tcp flags, 1 bit each */
#define get_fin(p)				((uint8_t)((p)[13])) & (1 << 0)
#define set_fin(p)				((uint8_t)((p)[13])) |= (1 << 0)
#define clear_fin(p)				((uint8_t)((p)[13])) &= ~(1 << 0)

#define get_syn(p)				((uint8_t)((p)[13])) & (1 << 1)
#define set_syn(p)				((uint8_t)((p)[13])) |= (1 << 1)
#define clear_syn(p)				((uint8_t)((p)[13])) &= ~(1 << 1)

#define get_rst(p)				((uint8_t)((p)[13])) & (1 << 2)
#define set_rst(p)				((uint8_t)((p)[13])) |= (1 << 2)
#define clear_rst(p)				((uint8_t)((p)[13])) &= ~(1 << 2)

#define get_ack(p)				((uint8_t)((p)[13])) & (1 << 4)
#define set_ack(p)				((uint8_t)((p)[13])) |= (1 << 4)
#define clear_ack(p)				((uint8_t)((p)[13])) &= ~(1 << 4)

typedef struct {
	machine_t *machine;
	unsigned int fd;
	short local_port;
	short remote_port;
	int remote_node;
	uint32_t seq_num;

} tcp_socket_t;

tcp_socket_t *socket_table[MAXSOCKETS];

void v_tcp_init();
int v_socket();
int v_bind(int socket, int node, short port);
int v_listen(int socket, int backlog /* optional */);
int v_connect(int socket, int node, short port);
int v_accept(int socket, int *node);
int v_read(int socket, unsigned char *buf, int nbyte);
int v_write(int socket, const unsigned char *buf, int nbyte);
int v_close(int socket);

#endif
