#include "seq.h"


int isValidSeqNum(tcp_socket_t *sock, int num) {
	return cbuf_btm_contains(sock->r_buf, sock->recv_next, num);
}

int haveRoomToReceive(tcp_socket_t *sock) {
	return sock->recv_window_size > 0;
}

/* call this when you send out an ACK packet, to update pointers 
 *	
 *	int size - size of the data we're acking
 * */
void ackData(tcp_socket_t *sock, int size) {
	sock->recv_next += size;
	sock->seq_num += size;
}


/*************************-=[ SENDING STUFFS ]=-********************/

int canAcceptDataToSend(tcp_socket_t *sock, size) {
	return (sock->send_una + sock->send_window_size - sock->send_next) > 0;
}

int getAmountAbleToSend(tcp_socket_t *sock) {
	return (sock->send_una + sock->send_window_size - sock->send_next);
}

