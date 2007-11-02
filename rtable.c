#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <htable.h>
#include <assert.h>
#include <van.h>
#include <pthread.h>

#include "rtable.h"

void rtable_cleanup(rtable_t* rt) {
  pthread_mutex_unlock(&rt->lock);
}

/* Returns 1 on table update, 0 if no change. */
int rtable_merge(int self_addr, rtable_t *local, rtable_t *remote, rtable_entry_t *conduit) {
  rtable_entry_t* r_entry;
  rtable_entry_t* l_entry;
  int cost, modify = 0;
  
  pthread_cleanup_push((void (*)(void*))rtable_cleanup, local);
  pthread_cleanup_push((void (*)(void*))rtable_cleanup, remote);
  pthread_mutex_lock(&local->lock);
  pthread_mutex_lock(&remote->lock);

  rtable_iterate_begin(remote, r_entry) {
    cost = conduit->cost + r_entry->cost;
	 cost = cost >= INFINITY ? INFINITY : cost;

    /* Don't add route to self. */
    if(self_addr == r_entry->addr || DEFAULT_ROUTE == r_entry->addr) {
      continue;
    }

    /* Lookup address in local table; if not found, simply insert. */
    if(!(l_entry = rtable_get(local, r_entry->addr))) {
      if(!(l_entry = malloc(sizeof(rtable_entry_t)))) {
        fprintf(stderr, "Merge failure.\n");
        continue;
      }

      /* Add entry to routing table. */
      l_entry->addr = r_entry->addr;
      rtable_put(local, l_entry);

    /* If our route's next hop is through the conduit, we must unconditionally update our tables. */
    } else if(l_entry->cost <= cost && l_entry->next_hop != conduit->addr) {
      continue;
    }
    
	 modify = !(l_entry->iface == conduit->iface && l_entry->next_hop == conduit->addr && l_entry->type == conduit->type && l_entry->cost == cost);

    /* Improve entry by traversing conduit or update if remote node is next hop. */
    l_entry->iface = conduit->iface;
    l_entry->next_hop = conduit->addr;
    l_entry->type = conduit->type;
    l_entry->cost = cost;

  } rtable_iterate_end();

  pthread_cleanup_pop(1);
  pthread_cleanup_pop(1);

  return modify;
}

rtable_t* rtable_new() {
  rtable_t* rt = (rtable_t*)malloc(sizeof(rtable_t));
  pthread_mutexattr_t attr;

  if(!rt) {
    return NULL;
  }

  /* Initialize underlying hashtable. */
  htable_init(&rt->hash, VAN_MAXNODES + 1);

  /* Create a recursive mutex for the routing table. */
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
  pthread_mutex_init(&rt->lock, &attr);
  
  return rt;
}

void rtable_destroy(rtable_t* rt) {
  rtable_entry_t* entry;

  /* Cleanup each routing entry. */
  rtable_iterate_begin(rt, entry) {
    free(entry);
  } rtable_iterate_end();

  /* Destroy the routing table. */
  htable_destroy(&rt->hash);

  /* Destroy the routing table lock. */
  pthread_mutex_destroy(&rt->lock);
}

rtable_entry_t* rtable_get(rtable_t *rt, int addr) {
	rtable_entry_t *r; 

  pthread_cleanup_push((void (*)(void*))rtable_cleanup, rt);
  pthread_mutex_lock(&rt->lock);

  r = (rtable_entry_t*)htable_get(&rt->hash, addr);

	if (r == NULL) {
		r = (rtable_entry_t*)htable_get(&rt->hash, DEFAULT_ROUTE);
	}

  pthread_cleanup_pop(1);
  
  return r;
}


void rtable_put(rtable_t *rt, rtable_entry_t* entry) {
  void* old;
  
  pthread_cleanup_push((void (*)(void*))rtable_cleanup, rt);
  pthread_mutex_lock(&rt->lock);

  /* Insert data and free old data. */
  if((old = htable_put(&rt->hash, entry->addr, entry))) {
    free(old);
  }

  pthread_cleanup_pop(1);
}

void rtable_remove(rtable_t *rt, int addr) {
  void* old;
  
  pthread_cleanup_push((void (*)(void*))rtable_cleanup, rt);
  pthread_mutex_lock(&rt->lock);

  /* Remove data from hash and free old data. */
  if((old = htable_remove(&rt->hash, addr))) {
    free(old);
  }

  pthread_cleanup_pop(1);
}

void rtable_set_cost(rtable_t *rt, int addr, int cost) {
	// TODO this function should be OK.
	
	rtable_entry_t *new = malloc(sizeof(rtable_entry_t));
	printf("Trying to set the cost for addr %d to %d\n", addr, cost);
	rtable_entry_t* old_entry = rtable_get(rt, addr);	

	old_entry->cost = cost;
	memcpy(new, old_entry, sizeof(rtable_entry_t));
	rtable_remove(rt, addr);
	rtable_put(rt, new);

	return;
}

void rtable_poison_iface(rtable_t *rt, int iface) {
  rtable_entry_t* entry;

  pthread_cleanup_push((void (*)(void*))rtable_cleanup, rt);
  pthread_mutex_lock(&rt->lock);

  /* First scan and then remove; cannot modify table while iterating. */
  rtable_iterate_begin(rt, entry) {
    if(entry->iface == iface) {
      entry->cost = INFINITY;
    }
  } rtable_iterate_end();

  pthread_cleanup_pop(1);
}

/* Debugging and Transmission. */
char* rtable_serialize(rtable_t* rt, int *len, int filter_addr) {
  char *bytes, *bptr;
  rtable_entry_t* entry;

  pthread_cleanup_push((void (*)(void*))rtable_cleanup, rt);
  pthread_mutex_lock(&rt->lock);

  bptr = bytes = malloc(*len = (sizeof(rtable_entry_t) * rt->hash.ht_size));

  if(bytes) {
    rtable_iterate_begin(rt, entry) {
      assert(bptr < bytes + *len);

      if(entry->next_hop != filter_addr) {
        bptr += rtable_entry_serialize(bptr, entry);
      }
    } rtable_iterate_end();
  }

  pthread_mutex_unlock(&rt->lock);
  pthread_cleanup_pop(1);


  return bytes;
}

rtable_t* rtable_unserialize(const char* bytes, int len) {
  const char* bptr = bytes;
  rtable_entry_t* entry;
  rtable_t* rt = rtable_new();

  if(!rt) {
    return NULL;
  }

  while(bptr - bytes < len) {
    bptr += rtable_entry_unserialize(bptr, &entry);
    rtable_put(rt, entry);
  }
  
  return rt;
}

int rtable_entry_serialize(char* bytes, rtable_entry_t* entry) {
  memcpy(bytes, entry, sizeof(rtable_entry_t));

  return sizeof(rtable_entry_t);
}

int rtable_entry_unserialize(const char* bytes, rtable_entry_t** entry) {
  *entry = (rtable_entry_t*)malloc(sizeof(rtable_entry_t));
  memcpy(*entry, bytes, sizeof(rtable_entry_t));

  return sizeof(rtable_entry_t);
}

void rtable_dump(rtable_t *rt) {
  rtable_entry_t *entry;

  pthread_cleanup_push((void (*)(void*))rtable_cleanup, rt);
  pthread_mutex_lock(&rt->lock);
  
  printf("----------------------\n");
  printf("Route table %p\n", rt);
  printf("----------------------\n");

  rtable_iterate_begin(rt, entry) {
    printf(" * \tAddress: %d, Iface: %d, Next: %d, Type: %d, Cost: %d\n",
      entry->addr, entry->iface, entry->next_hop, entry->type, entry->cost);                                                                          
  } rtable_iterate_end();

  pthread_cleanup_pop(1);
}

