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

socksplit_t* alloc_ssplit(tcp_socket_t *sock) {
  socksplit_t* split = malloc(sizeof(socksplit_t));

  /* Ensure success. */

  split->half = sock;
  split->full = sock ? NULL : alloc_htable();

  return split;
}

void socktable_init(socktable_t *st) {
  htable_init(&st->root, MAXSOCKETS);
}

void socktable_destroy(socktable_t *st) {
  htable_t *lport_h, *rport_h;
  socksplit_t *rsplit;

  /* Iterate through all hash tables; delete appropriately. */
  htable_iterate_begin(&st->root, lport_h, htable_t) { 
    htable_iterate_begin(lport_h, rsplit, socksplit_t) {
      if(rsplit->full) {
        htable_iterate_begin(rsplit->full, rport_h, htable_t) {
        
          /* Destroy remote port htable (DO NOT free sockets). */
          htable_destroy(rport_h);
          free(rport_h);

        } htable_iterate_end();

        /* Destroy remote node htable. */
        htable_destroy(rsplit->full);
        free(rsplit->full);
      }
      
      if(rsplit->half) {
        free(rsplit->half);
      }

      free(rsplit);
    } htable_iterate_end();
    /* Destroy local port htable. */
    htable_destroy(lport_h);
    free(lport_h);

  } htable_iterate_end();

  /* Destroy local host htable. */
  htable_destroy(&st->root);
}

tcp_socket_t *socktable_get(socktable_t *st, int lnode, short lport, int rnode, short rport, uint8_t full) {
  htable_t *lport_h, *rport_h;
  socksplit_t *rsplit;
  tcp_socket_t *result = NULL;

  /* Perform four constant time lookups. */
  if((lport_h = htable_get(&st->root, lnode))) {
    if((rsplit = htable_get(lport_h, lport))) {
      if(!full) {
        result = rsplit->half;
      } else if(rsplit->full && (rport_h = htable_get(rsplit->full, rnode))) {
        result = htable_get(rport_h, rport);
      }
    }
  }

  return result;
}

tcp_socket_t *socktable_put(socktable_t *st, tcp_socket_t *data, uint8_t full) {
  htable_t *lport_h, *rport_h;
  socksplit_t *rsplit;
  int fresh = 0;

  /* (Possibly) create all hash tables along path to socket. */
  if((lport_h = htable_get(&st->root, data->local_node->van_node->vn_num)) == NULL) {
    htable_put(&st->root, data->local_node->van_node->vn_num, lport_h = alloc_htable());
    fresh = 1;
  }

  if(fresh || (rsplit = htable_get(lport_h, data->local_port))) {
    void* rval = htable_put(lport_h, data->local_port, rsplit = alloc_ssplit(full ? NULL : data));
    fresh = 1;

    if(!full) {
      return rval;
    }
  }

  if(fresh || (rport_h = htable_get(rsplit->full, data->remote_node))) {
    htable_put(rsplit->full, data->remote_node, rport_h = alloc_htable());
  }

  return htable_put(rport_h, data->remote_port, data);
}

tcp_socket_t *socktable_remove(socktable_t *st, int lnode, short lport, int rnode, short rport, uint8_t full) {
  htable_t *lport_h, *rport_h;
  socksplit_t *rsplit;

  tcp_socket_t *result = NULL;

  if((lport_h = htable_get(&st->root, lnode))) {
    if((rsplit = htable_get(lport_h, lport))) {
      if(!full) {
        result = rsplit->half;
        rsplit->half = NULL;

      } else if((rport_h = htable_get(rsplit->full, rnode))) {
        result = htable_remove(rport_h, rport);
      }
    }
  }

  return result;
}

tcp_socket_t *socktable_promote(socktable_t *st, tcp_socket_t *data) {
  tcp_socket_t *result = NULL;

  if(socktable_remove(st, data->local_node->van_node->vn_num, data->local_port, data->remote_node, data->remote_port, 0)) {
    result = socktable_put(st, data, 1);
  }

  return result;
}

void socktable_dump(socktable_t *st, uint8_t full) {
  tcp_socket_t* sock;

  socktable_iterate_begin(st, sock, full) {
    printf("%s Socket [lnode = %d, lport = %d, rnode = %d, rport = %d]\n",
      full ? "Full" : "Half", sock->local_node->van_node->vn_num, sock->local_port, sock->remote_node, sock->remote_port);
  } socktable_iterate_end();
}
