#ifndef __SEQ_H_
#define __SEQ_H_

#include "tcp.h"
#include "cbuffer.h"

/*           
 *            [unack    /[next    /[writen    /[
 *           /         /         /      ____,/
 *          /         /         /      /
 * +-------+---------+---------+------+----------------
 * | ACKd  | unacked | written | xmit |         |
 * +-------+---------+---------------------------------
 *
 */

#define MIN(a,b)	(a < b ? a: b)

int isValidSeqNum(ip_socket_t *sock, int num);
int haveRoomToReceive(tcp_socket_t *sock);
void ackData(tcp_socket_t *sock, int size);
int dataToRead(tcp_socket_t *sock);
int getDataFromBuffer(tcp_socket_t *sock, char *buf, int max_size);

int canAcceptDataToSend(tcp_socket_t *sock, size);
int getAmountAbleToSend(tcp_socket_t *sock);
int gotAckFor(tcp_socket_t *sock, int start, int len);
int copyDataToBuffer(tcp_socket_t *sock, char *data, int size);

#endif
