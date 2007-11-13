#ifndef RTABLE_H
#define RTABLE_H

#include <htable.h>
#include <pthread.h>

#define DEFAULT_ROUTE VAN_MAXNODES
#define INFINITY 16
#define RIP_SLEEP 30
#define LINK_TIMEOUT	61

/* Route table entry. */
typedef struct {
  int addr; // redundant, but makes serializing routing table much easier.
  int iface;
  int next_hop;
  int type;
  int cost;
} rtable_entry_t;

/* Hide implementation of route table. */
typedef struct {
  htable_t hash;
  pthread_mutex_t lock;
} rtable_t;

/* Constructor / Destructor. */
rtable_t* rtable_new();
void rtable_destroy(rtable_t* rt);

/* Operations. */
rtable_entry_t* rtable_get(rtable_t *rt, int addr);
void rtable_put(rtable_t *rt, rtable_entry_t* entry);
void rtable_remove(rtable_t *rt, int addr);
void rtable_poison_iface(rtable_t *rt, int iface);
int rtable_merge(int self_addr, rtable_t *local, rtable_t *remote, rtable_entry_t* conduit);

/* Debugging and Transmission. */
char* rtable_serialize(rtable_t *rt, int *len, int filter_addr);
rtable_t* rtable_unserialize(const char* bytes, int len);

int rtable_entry_serialize(char* bytes, rtable_entry_t* entry);
int rtable_entry_unserialize(const char* bytes, rtable_entry_t** entry);

void rtable_dump(rtable_t *rt);

/* Cleanup handlers. */
void rtable_cleanup(rtable_t* rtable);

/* Iteration. */
#define rtable_iterate_begin(rt, entry_var)     do { pthread_cleanup_push((void(*)(void*))rtable_cleanup, rt) \
                                                     pthread_mutex_lock(&rt->lock); \
                                                     htable_iterate_begin(&((rt)->hash), entry_var, rtable_entry_t)
#define rtable_iterate_end()                    htable_iterate_end(); pthread_cleanup_pop(1); } while(0)

#endif
