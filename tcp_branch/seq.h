#ifndef __SEQ_H_
#define __SEQ_H_

#include "tcp.h"
#include "cbuffer.h"

/*           
 *            [unack    /[next    /[writen    /[una + windowsize
 *           /         /         /      ____,/
 *          /         /         /      /
 * +-------+---------+---------+------+----------------
 * | ACKd  | unacked | written | xmit |         |
 * +-------+---------+---------------------------------
 *
 */

#define MIN(a,b)	(a < b ? a: b)

int isOldSeqNum(tcp_socket_t *sock, unsigned int num, unsigned int size, char* packet);
int isValidSeqNum(tcp_socket_t *sock, unsigned int num, unsigned int size, char* packet);
int isNextSeqNum(tcp_socket_t *sock, int unsigned num);
int haveRoomToReceive(tcp_socket_t *sock, unsigned int size);
void ackData(tcp_socket_t *sock, int size);
int amountOfDataToRead(tcp_socket_t *sock);
int getDataFromBuffer(tcp_socket_t *sock, char *buf, int max_size);
int amountWeCanReceive(tcp_socket_t *sock);
int dataFromNetworkToBuffer(tcp_socket_t *sock, char *data, unsigned int size);
void updateFromWindowAnnounce(tcp_socket_t *sock, int window);
void processPacketForAck(tcp_socket_t *sock, char*packet);
int sendFlagNext(tcp_socket_t *sock);
int getFlagToSend(tcp_socket_t *sock);

int canAcceptDataToSend(tcp_socket_t *sock, int size);
int getAmountAbleToSend(tcp_socket_t *sock);
int getAmountAbleToAccept(tcp_socket_t *sock);
void gotAckFor(tcp_socket_t *sock, int start /*, int len*/);
int copyDataFromUser(tcp_socket_t *sock,  char *data, int size);
void unackData(tcp_socket_t *sock, int size);
int dataFromBufferToNetwork(tcp_socket_t *sock, char *data, int size);

#endif
