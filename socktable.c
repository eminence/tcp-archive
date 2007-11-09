#include <htable.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "socktable.h"
#include "tcp.h"
#include "tcppacket.h"

htable_t* alloc_htable() {
  htable_t* result = malloc(sizeof(htable_t));

  /* Ensure success. */
  assert(result);
  htable_init(result, MAXSOCKETS);

  return result;
}

void socktable_init(socktable_t *st) {
  htable_init(&st->root, MAXSOCKETS);
}

void socktable_destroy(socktable_t *st) {
  htable_t *lport_h, *rnode_h, *rport_h;

  /* Iterate through all hash tables; delete appropriately. */
  htable_iterate_begin(&st->root, lport_h, htable_t) { 
    htable_iterate_begin(lport_h, rnode_h, htable_t) {
      htable_iterate_begin(rnode_h, rport_h, htable_t) {
        
        /* Destroy remote port htable (DO NOT free sockets). */
        htable_destroy(rport_h);
      } htable_iterate_end();

      /* Destroy remote node htable. */
      htable_destroy(rnode_h);
    } htable_iterate_end();

    /* Destroy local port htable. */
    htable_destroy(lport_h);
  } htable_iterate_end();

  /* Destroy local host htable. */
  htable_destroy(&st->root);
}

tcp_socket_t *socktable_get(socktable_t *st, int lnode, short lport, int rnode, short rport) {
  htable_t *lport_h, *rnode_h, *rport_h;
  tcp_socket_t *result = NULL;

  /* Perform four constant time lookups. */
  if((lport_h = htable_get(&st->root, lnode))) {
    if((rnode_h = htable_get(lport_h, lport))) {
      if((rport_h = htable_get(rnode_h, rnode))) {
        result = htable_get(rport_h, rport);
      }
    }
  }

  return result;
}

tcp_socket_t *socktable_put(socktable_t *st, tcp_socket_t *data) {
  htable_t *lport_h, *rnode_h, *rport_h;
  int fresh = 0;

  /* (Possibly) create all hash tables along path to socket. */
  if((lport_h = htable_get(&st->root, data->local_node->van_node->vn_num)) == NULL) {
    htable_put(&st->root, data->local_node->van_node->vn_num, lport_h = alloc_htable());
    fresh = 1;
  }

  if(fresh || (rnode_h = htable_get(lport_h, data->local_port))) {
    htable_put(lport_h, data->local_port, rnode_h = alloc_htable());
    fresh = 1;
  }

  if(fresh || (rport_h = htable_get(rnode_h, data->remote_node))) {
    htable_put(rnode_h, data->remote_node, rport_h = alloc_htable());
  }

  return htable_put(rport_h, data->remote_port, data);
}

tcp_socket_t *socktable_remove(socktable_t *st, int lnode, short lport, int rnode, short rport) {
  htable_t *lport_h, *rnode_h, *rport_h;
  tcp_socket_t *result = NULL;

  /* Perform four constant time lookups. */
  if((lport_h = htable_get(&st->root, lnode))) {
    if((rnode_h = htable_get(lport_h, lport))) {
      if((rport_h = htable_get(rnode_h, rnode))) {
        result = htable_remove(rport_h, rport);
      }
    }
  }

  return result;
}

void socktable_dump(socktable_t *st) {
  tcp_socket_t* sock;

  socktable_iterate_begin(st, sock) {
    printf("Socket [lnode = %d, lport = %d, rnode = %d, rport = %d]\n",
      sock->local_node->van_node->vn_num, sock->local_port, sock->remote_node, sock->remote_port);
  } socktable_iterate_end();
}
