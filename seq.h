#ifndef __SEQ_H_
#define __SEQ_H_

#include "tcp.h"
#include "cbuffer.h"

int isValidSeqNum(ip_socket_t *sock, int num);
int haveRoomToReceive(tcp_socket_t *sock);
void ackData(tcp_socket_t *sock, int size);
int canAcceptDataToSend(tcp_socket_t *sock, size);
int getAmountAbleToSend(tcp_socket_t *sock);

#endif
