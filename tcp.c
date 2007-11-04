#include "tcp.h"
#include "van_driver.h"
#include "fancy_display.h"


/* returns a new unbound socket.
 * on failure, returns a negative value */
int v_socket() {

	return 0;
}

/* binds a socket to a port
 * returns 0 on success or negative number on failure */
int v_bind(int socket, int node, short port) {

	return 0;
}

/* moves a socket into listen state
 * returns 0 on success or negative number on failure */
int v_listen(int socket, int backlog /* optional */) {

	return 0;
}


/* connects a socket to an address
 * returns 0 on success of a negative number on failure */
int v_connect(int socket, int node, short port) {

	return 0
}

/* accept a requested connection
 * returns new socket handle on success or negative number on failure */
int v_accept(int socket, int *node) {

	return 0;
}


/* read on an open socket
 * return num types read or negative number on failure or 0 on EOF */
int v_read(int socket, unsigned char *buf, int nbyte) {

	return 0;
}

/* write on an open socket
 * retursn num bytes written or negative number on failure */
int v_write(int socket, const unsigned char *buf, int nbyte) {

	return 0;
}

/* close an open socket
 * returns 0 on success, or negative number on failure */
int v_close(int socket) {

	return 0;
}
