#include <stdlib.h>

#include "seq.h"

/* RECEVING FUNCTIONS */

int isValidSeqNum(tcp_socket_t *sock, int num) {
	return cbuf_btm_contains(sock->r_buf, sock->recv_next, sock->recv_next + sock->recv_window_size, num);
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
int getDataFromBuffer(tcp_socket_t *sock, char *buf, int max_size) {
	assert(buf);
	int amount = MIN(max_size, amountOfDataToRead(sock));

	assert(amount > 0);

	char *memory = cbuf_get_range(sock->r_buf, sock->recv_read, amount);
	memcpy(buf, memory, amount);
	free(memory);

	sock->recv_read += amount;
	sock->recv_window_size += amount;
	assert(sock->recv_read <= sock->recv_next);

	
	return amount;

}

/* return the amount of data we can write to in our cbuffer */
int amountWeCanReceive(tcp_socket_t *sock) {
	return sock->recv_window_size;
}

/* copy data from network into our cbuffer */
int dataFromNetworkToBuffer(tcp_socket_t *sock, char *data, int size) {
	assert (size <= sock->recv_window_size);
	
	cbuf_put_range(sock->r_buf, data, sock->recv_next, size);

	return 0;
}


/*************************-=[ SENDING STUFFS ]=-********************/

/* when we get a window announcement, pass the window into this function */
void updateFromWindowAccounce(tcp_socket_t *sock, int window) {
	sock->remote_flow_window = sock->send_next + window;

}

/* do we have room in our cbuffer to accept new data to send from the user */
int canAcceptDataToSend(tcp_socket_t *sock, int size) {
	return (sock->send_una + sock->send_window_size - sock->send_written) > 0;
}

/* return the amount of data we have in our cbuff that we can send out in a tcp packet */
int getAmountAbleToSend(tcp_socket_t *sock) {
	return MIN(  /* make sure we have data actually written before we try to send it */
			sock->send_written - sock->send_next,
			sock->remote_flow_window - sock->send_next);
}

/* call this when we have gotten an ACK for a packet */
void gotAckFor(tcp_socket_t *sock, int start, int len) {
	//assert(sock->send_una = start);  /* not true, because acks are consecutive */
	sock->send_una += len;

	assert(sock->send_una <= sock->send_next);
}

/* take data from the user and copy it into our cbuffer */
int copyDataFromUser(tcp_socket_t *sock, char *data, int size) {
	assert(canAcceptDataToSend(sock, size));
	assert(sock->send_written >= sock->send_next); 
	assert(sock->send_written + size <= sock->send_una + sock->send_window_size);

	cbuf_put_range(sock->s_buf, data, sock->send_written, size);
	sock->send_written += size;
	return 0;
}

/* true if we have data ready to send */
int haveDataToSend(tcp_socket_t *sock) {
	return (getAmountAbleToSend(sock) > 0);
}

/* if we get a duplicate ack, we bump bback send_next */
void unackData(tcp_socket_t *sock, int size) {
	sock->send_next = sock->send_una;
}


/* take data from our cbuffer, and give it to the network */
int dataFromBufferToNetwork(tcp_socket_t *sock, char *data, int size) {
	
	int amount = getAmountAbleToSend(sock);

	char * memory = cbuf_get_range(sock->s_buf, sock->send_next, amount);
	memcpy(data, memory, amount);
	free(memory);
	
	return amount;

}
