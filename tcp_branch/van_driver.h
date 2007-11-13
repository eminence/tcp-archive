#ifndef _VAN_DRIVER
#define _VAN_DRIVER

#include <van.h>
#include <pthread.h>
#include <bqueue.h>
#include "rtable.h"
#include "tcp.h"
#include "socktable.h"
#include "fancy_display.h"
#include "ippacket.h"
#include "tcppacket.h"

struct tcp_socket__;
struct socktable__;

typedef struct {
	unsigned char cur_state;
	unsigned char old_state;
	int peer; /* address of peer on the other end of this link */
	time_t age; /* time when last packet was recieved */
	unsigned char timed_out;
	pthread_cond_t cond;
	pthread_mutex_t lock;
	pthread_mutex_t age_lock;
} iface_t;

typedef struct ip_node__ {
	van_node_t *van_node;
	pthread_t *sending_thread;
	pthread_t *listening_thread[VAN_MAXINTERFACES];
	pthread_t *rip_thread;
	pthread_t *rip_monitor_thread;
	pthread_t *link_state_thread;
	bqueue_t *sending_q;
	bqueue_t *receiving_q; /* this may not beed needed any more! */
	bqueue_t *rip_q;
	bqueue_t *tcp_q;
	pthread_t *tcp_thread;
	pthread_t *tcp_send_thread;
	pthread_t *tcp_watchdog;
  
  /* Let the fun begin. */
  struct tcp_socket__ *socket_table[MAXSOCKETS];
  struct socktable__ *tuple_table;

	rtable_t *route_table;
	iface_t *ifaces;

} ip_node_t;


typedef struct {
	ip_node_t *node;
	int iface;
} node_and_num_t;

typedef struct {
	int iface;
	char *packet;
	int packet_size;
	unsigned int addr_type;
} ip_packet_t;

typedef struct {
	int iface;
	char *packet;
} rip_packet_t;

int van_driver_loaded();

ip_node_t *van_driver_init(char *fname, int num);
void van_driver_destory(ip_node_t *node);

int get_if_state(ip_node_t *node, int iface);
int set_if_state(ip_node_t *node, int iface, int state);
int van_driver_sendto (ip_node_t *node, char *buf, int size, int to, uint8_t proto);
int van_driver_recvfrom (ip_node_t *node, char *buf, int size) ;

/* High level functionality. */
void entry_to_vaddr(rtable_entry_t* entry, vanaddr_t* vaddr);
rtable_entry_t* lookup_route(ip_node_t* node, int addr);
int add_route(ip_node_t* node, int addr, int iface, int cost, int next_hop);     /* TODO: Expand for RIP. */
int update_route(ip_node_t *node, int addr, int iface, int cost, int next_hop);

#endif
