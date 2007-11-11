#include "seq.h"

/* RECEVING FUNCTIONS */

int isValidSeqNum(tcp_socket_t *sock, int num) {
	return cbuf_btm_contains(sock->r_buf, sock->recv_next, num);
}

/* do we have room to copy incoming data into our cbuffer? */
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


/* returns the amount of data in our cbuffer we can pass up to the layer above */
int amountOfDataToRead(tcp_socket_t *sock) {
	assert(sock->recv_next >= sock->recv_read);

	return (sock->recv_next - sock->recv_read);
}

/* copy memory from our cbuffer into a user specified buffer */
int getDataFromUser(tcp_socket_t *sock, char *buf, int max_size) {
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

/* do we have room in our cbuffer to accept new data to send from the user */
int canAcceptDataToSend(tcp_socket_t *sock, size) {
	return (sock->send_una + sock->send_window_size - sock->send_written) > 0;
}

/* return the amount of data we have in our cbuff that we can send out in a tcp packet */
int getAmountAbleToSend(tcp_socket_t *sock) {
	return MIN(  /* make sure we have data actually written before we try to send it */
			sock->send_written - sock->send_next,
			sock->send_una + sock->send_window_size - sock->send_next);
}

/* call this when we have gotten an ACK for a packet */
int gotAckFor(tcp_socket_t *sock, int start, int len) {
	//assert(sock->send_una = start);  /* not true, because acks are consecutive */
	sock->send_una += len;

	assert(sock->send_una <= sock->send+next);
}

/* take data from the user and copy it into our cbuffer */
int copyDataFromUser(tcp_socket_t *sock, char *data, int size) {
	assert(canAcceptDataToSend(sock, size));
	assert(sock->send_written >= sock->send_next); 
	assert(sock->send_written + size <= send_una + send_window);

	cbuf_put_range(sock->s_buf, data, sock->send_written, size);
	sock->send_written += size;
}

/* true if we have data ready to send */
int haveDataToSend(tcp_socket_t *sock) {
	return (getAmountAbleToSend(sock) > 0);
}

/* if we get a duplicate ack, we bump bback send_next */
void unackData(tcp_socket_t *sock, int size) {
	sock->send_next = sock->send_una;
}
