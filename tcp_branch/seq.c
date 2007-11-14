#include <stdlib.h>

#include "seq.h"
#include "tcppacket.h"

/* RECEVING FUNCTIONS */


int isOldSeqNum(tcp_socket_t *sock, int num, int size, char* packet) {
	return cbuf_lt(sock->r_buf, num, sock->recv_next); // return true if seqnum preceeds our current sequence number
}

int isValidSeqNum(tcp_socket_t *sock, int num, int length, char* packet) {
	/* if BOTH num and (num + length) is in the range that we're willing to receive */
	nlog(MSG_LOG, "isValidSeqNum", "sock->recv_next=%d, sock->recv_window_size=%d, num=%d, length=%d", sock->recv_next, sock->recv_window_size, num, length);

	return cbuf_btm_contains(sock->r_buf, sock->recv_next, sock->recv_next + sock->recv_window_size, num) &&
		cbuf_btm_contains(sock->r_buf, sock->recv_next, sock->recv_next + sock->recv_window_size, num+length);

}

/* return true is this sequence number is the next one we're expecting */
int isNextSeqNum(tcp_socket_t *sock, int num) {
	return (num == sock->recv_next);
}



/* do we have room to copy incoming data into our cbuffer? */
int haveRoomToReceive(tcp_socket_t *sock, int size) {
	return sock->recv_window_size >= size;
}

/* call this when you send out an ACK packet, to update pointers 
 *	
 * This updates the ACK we reply with in the subsequent packet!
 * I.E. This ACKs data up to previous ack + size (+1 to be next expected)
 *
 *	int size - size of the data we're acking
 * */
void ackData(tcp_socket_t *sock, int size) {
	nlog(MSG_WARNING, "ackData", "bumping recv_next from %d to %d", sock->recv_next, sock->recv_next + size);
	sock->recv_next += size;
	sock->seq_num += size; // XXX seq_num is obsolete and meaningless
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
	char *d;
	int toreturn;

	int retval = cbuf_get_range(sock->r_buf, sock->recv_read, amount, &d);
	if (retval < 0) {
		assert(0 && "Got a CONTROL FLAG in our recv buffer!");
		/* this is a flag */
		//toreturn = *(int*)d;  /* these are the flags */
		toreturn = 0;

	} else {
		/* Check if we read a (special) EOF */
		if(!d) {
			return -1;
		}

		memcpy(buf, d, retval);

		sock->recv_read += amount;
		sock->recv_window_size += amount;
		assert(sock->recv_read <= sock->recv_next);
		toreturn = amount;
	}

	free(d); /*hazzzxxxxxxx*/

	
	return toreturn;

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
void updateFromWindowAnnounce(tcp_socket_t *sock, int window) {
	sock->remote_flow_window = sock->send_next + window;
		/* XXX we might want to use send_una instead of send_next */
}

/* do we have room in our cbuffer to accept new data to send from the user */
int canAcceptDataToSend(tcp_socket_t *sock, int size) {
	return (sock->send_una + sock->send_window_size - sock->send_written) > 0;
}

/*  return the amount of data we can accept into our cbuffer */
int getAmountAbleToAccept(tcp_socket_t *sock) {
	return (sock->send_una + sock->send_window_size - sock->send_written);

}


/* return true if the next thing to send is a flag */
int sendFlagNext(tcp_socket_t *sock) {

	void *d;

	int toreturn;

	/* if there is nothing to be sent, return 0 */
	if (sock->send_written == sock->send_next) return 0;

	int retval = cbuf_get_range(sock->s_buf, sock->send_next, 1, &d);
	if (retval < 0) {
		/* this is a flag */
		//toreturn = *(int*)d;  /* these are the flags */
		toreturn = 1;

	} else {
		assert(d); // we should never get an EOF in our send buffer!
		toreturn = 0;
	}
	
	free(d); /*hazzzxxxxxxx*/
	
	return toreturn;
}


/* get the flag to send next */
int getFlagToSend(tcp_socket_t *sock) {

	assert(sendFlagNext(sock));

	void *d;

	int toreturn;
	int retval = cbuf_get_range(sock->s_buf, sock->send_next, 1, &d);
	if (retval < 0) {
		/* this is a flag */
		toreturn = *(int*)d;  /* these are the flags */
	} else {
		assert(retval < 0); // XXX PANIC
		assert(d); // ensure that we didn't get an EOF flag in our send buffer
	}
	
	free(d); /*hazzzxxxxxxx*/

	return toreturn;

}

/* return the amount of data we have in our cbuff that we can send out in a tcp packet */
int getAmountAbleToSend(tcp_socket_t *sock) {
	int m = MIN(  /* make sure we have data actually written before we try to send it */
				sock->send_written - sock->send_next,
				sock->remote_flow_window - sock->send_next);

	assert(m >= 0);

	if (m == 0) return 0; /* skip all this crap and just return zero */

	void *d;

	int toreturn;

	if (sock->send_next == sock->send_written) return 0;

	int retval = cbuf_get_range(sock->s_buf, sock->send_next, m, &d);
	if (retval < 0) {
		/* this is a flag */
		//toreturn = *(int*)d;  /* these are the flags */
		toreturn = 1;

	} else {
		assert(d); // ensure that we didn't just read an EOF in our send buffer-- this is illegal
		toreturn = retval;
	}

	free(d); /*hazzzxxxxxxx*/

	return retval;

}

int isDupAck(tcp_socket_t *sock, int num) {

	return (num == sock->last_ack);
}



/* process  this packet for ACK stufffs 
 * move una forward cause we just ACKd some SHIZNIT
 */
void processPacketForAck(tcp_socket_t *sock, char*packet) {

	int flags = get_flags(ip_to_tcp(packet));
	if ((flags & TCP_FLAG_ACK) == TCP_FLAG_ACK) {
		int ack_num = get_acknum(ip_to_tcp(packet));
		if (ack_num > sock->send_una) {
			nlog(MSG_LOG, "processPacketForAck", "This packet has acknum=%d, which moves forward our send_una pointer (which was at %d)", ack_num, sock->send_una);
			sock->send_una = ack_num;
		} else {
			if (ack_num == sock->last_ack) {
				/* commenting out the next 2 lines as a TEMPORARY fix */
				//nlog(MSG_LOG, "processPacketForAck", "This packet is a duplicate ack.  we should move send_next back to send_una");
				//sock->send_next = sock->send_una;
			}
		}
		sock->last_ack = ack_num;
	}

}

/* call this when we have gotten an ACK for a packet */
void gotAckFor(tcp_socket_t *sock, int num /*, int len*/) {
	int dontUseThisFunction = 0;
	assert(dontUseThisFunction);

	//assert(sock->send_una = start);  /* not true, because acks are consecutive. uhh what?  need to think about this some more */
	nlog(MSG_LOG, "gocAckFor", "moving send_una from %d to %d", sock->send_una, num);
	sock->send_una = num;
	sock->last_ack = num;

	assert(sock->send_una <= sock->send_next);
}

/* take data from the user and copy it into our cbuffer */
int copyDataFromUser(tcp_socket_t *sock, const char *data, int size) {
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


/* take data from our cbuffer, and give it to the network, and move up send_next */
int dataFromBufferToNetwork(tcp_socket_t *sock, char *data, int size) {
	
	int amount = getAmountAbleToSend(sock);
	int toreturn;
	char *d;

	int retval = cbuf_get_range(sock->s_buf, sock->send_next, amount, &d);
	if (retval < 0) {
		/* this is a flag */
		//toreturn = *(int*)d;  /* these are the flags */
		toreturn = 0;

	} else {
		assert(d); // ensure that we didn't just read an EOF to send across network; illegal
		memcpy(data, d, retval);
		
		assert(retval == size);
		toreturn = retval;

	}

	free(d);

	return toreturn;
}


