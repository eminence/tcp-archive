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


/* returns the amount of data we can pass up to the layer above */
int dataToRead(tcp_socket_t *sock) {
	assert(sock->recv_next >= sock->recv_read);

	return (sock->recv_next - sock->recv_read);
}

int getDataFromBuffer(tcp_socket_t *sock, char *buf, int max_size) {
	assert(buf);
	int amount = MIN(max_size, dataToRead(sock));

	assert(amount > 0);

	char *memory = cbuf_get_range(sock->r_buf, sock->recv_read, amount);
	memcpy(buf, memory, amount);
	free(memory);

	sock->recv_read += amount;
	assert(sock->recv_read <= sock->recv_next);
	
	return amount;

}


/*************************-=[ SENDING STUFFS ]=-********************/

int canAcceptDataToSend(tcp_socket_t *sock, size) {
	return (sock->send_una + sock->send_window_size - sock->send_next) > 0;
}

int getAmountAbleToSend(tcp_socket_t *sock) {
	return MIN(  /* make sure we have data actually written before we try to send it */
			sock->send_written - sock->send_next,
			sock->send_una + sock->send_window_size - sock->send_next);
}

int gotAckFor(tcp_socket_t *sock, int start, int len) {
	assert(sock->send_una = start);
	sock->send_una += len;

	assert(sock->send_una <= sock->send+next);
}

int copyDataToBuffer(tcp_socket_t *sock, char *data, int size) {
	assert(canAcceptDataToSend(sock, size));
	assert(sock->send_written >= sock->send_next); 

	cbuf_put_range(sock->s_buf, data, sock->send_written, size);
	sock->send_written += size;
}

int haveDataToSend(tcp_socket_t *sock) {
	return (getAmountAbleToSend(sock) > 0);
}
