/*
 * main.c
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <van.h>
#include <assert.h>
#include <pthread.h>
#include <bqueue.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <checksum.h>
#include <time.h>
#include <stdarg.h>

#include <curses.h>
#include <panel.h>
#include <menu.h>

#include "rtable.h"
#include "van_driver.h"
#include "ippacket.h"
#include "tcp.h"
#include "socktable.h"

#include "fancy_display.h"

/* proto types */
int buildPacket(ip_node_t *node, char *data, int data_size, int to, char  **header, uint8_t proto);
void send_route_table(ip_node_t *node);
void bqueue_poorly_implemented_cleanup(bqueue_t* queue);

static char *types[] = {
    "INVAL",
    "ETH",
    "ATM",
    "P2P",
    "WIRELESS",
    "LOOP"
};

/*
 * The bqueue doesn't push a cleanup handler, so cancellation doesn't
 * release the mutex as expected.
 */
void bqueue_poorly_implemented_cleanup(bqueue_t* queue) {
#ifdef __LOSER__
	static int atm = 0xDEADBEEF;
#endif
	//nlog(MSG_LOG,"cleanup","Woah!  Manually unlocking a muxtexerthigny");
	//fflush(stdout);

	queue->q_count = 0;
	pthread_mutex_unlock(&queue->q_mtx);
}


/* print_packet
 * hex dump of a packet
 */
void print_packet (char *buf, int len) {
	int i, j;

	printf("\n--\nPacket Dump:\n--\n");

	for (i = 0; i < 32; i++) {
	printf("%02d ",i);
	}
	printf("\n\n");

	for (i = 0; i < len; i++) {
		for (j = 7; j >= 0; j--) {	
			unsigned char e = buf[i] & (1 << j); //d has the byte value.
			printf("%X  ", e>0?1:0);
		}
		if ((i+1)%4 == 0) printf("\n");
	}

	printf("\n--\n");
}

/*
 * returns the state of a link
 */
int get_if_state(ip_node_t *node, int iface) {
	int link_state;

	van_node_getifopt(node->van_node, iface, VAN_IO_UPSTATE, (char*)&link_state, sizeof(int));

	return link_state;
}



/*
 * sets the state of a link
 */
int set_if_state(ip_node_t *node, int iface, int link_state) {

	van_node_setifopt(node->van_node, iface, VAN_IO_UPSTATE, (char*)&link_state, sizeof(int));

	return link_state;
}

/* tcp send thread */
void *tcp_send_thread(void* arg) {
  pthread_exit(NULL);
}

/* tcp thread */
void *tcp_thread(void* arg) {
	ip_node_t *node = (ip_node_t*)arg;
  tcp_socket_t *old_sock;
	char *packet;
	uint16_t src_port;
	uint16_t dest_port;
	uint8_t src;
	uint8_t dest;
	uint8_t flags;

	while (1) {
		pthread_cleanup_push((void(*)(void*))bqueue_poorly_implemented_cleanup, node->tcp_q);
		bqueue_dequeue(node->tcp_q, (void*)&packet); /* this will block */
		pthread_cleanup_pop(0);

		nlog(MSG_LOG,"tcp", "tcp thread dequeued a tcp packet");

		src_port = get_srcport(ip_to_tcp(packet));
		dest_port = get_destport(ip_to_tcp(packet));
		src = get_src(packet);
		dest = get_dst(packet);
		flags = get_flags(ip_to_tcp(packet));

		nlog(MSG_LOG, "tcp", "source = %d, dest = %d, src_port = %d, dest_port = %d, flags = %s%s%s%s , window = %d, len = %d, seqnum=%d, acknum=%d",
			src, dest, src_port, dest_port, flags & TCP_FLAG_SYN ? " SYN" : "", flags & TCP_FLAG_ACK ? " ACK" : "", flags & TCP_FLAG_RST ? " RST" : "", flags & TCP_FLAG_FIN ? " FIN" : "", get_window(ip_to_tcp(packet)), get_data_len(packet), get_seqnum(ip_to_tcp(packet)), get_acknum(ip_to_tcp(packet)));

		tcp_socket_t *sock = socktable_get(node->tuple_table, dest, dest_port, src, src_port, FULL_SOCKET);
		//nlog(MSG_LOG,"tcp_thread", "dest=%d, dest_port=%d, src=%d, src_port=%d flags=%d", dest, dest_port, src, src_port, flags);
		//nlog(MSG_LOG,"tcp_thread", "about to dump socktable in tcp_thread");
		//socktable_dump(node->tuple_table, FULL_SOCKET);      
		//socktable_dump(node->tuple_table, HALF_SOCKET);

		/* If no match, may still be valid; ensure socket not listening on requested port. */
		if (sock == NULL) {
			nlog(MSG_WARNING, "tcp_thread", "We got a tcp packet, but it doesn't seem to have a full socket associated with it.  Halfchecking...");

			sock = socktable_get(node->tuple_table, dest, dest_port, 0, 0, HALF_SOCKET);

			/* Pwnz0r. */
			if (sock == NULL) {
				nlog(MSG_ERROR,"tcp_thread", "Ok, not a half socket either.  Discarding.");
				assert(packet);
				free(packet);

				continue;
			} 

			/* TODO: THIS IS WRONG!! AFTER FINDING HALF SOCK, NEED TO SLEEP UNTIL ACCEPT IS CALLED (IF IT HASNT ALREADY BEEN CALLED.*/

			/* Found half-socket. Ensure ready for accept. */
			if(!sock->can_handshake) {
				nlog(MSG_ERROR, "tcp_thread", "Found a half socket, but socket not in accept state. Discarding.");
				assert(packet);
				free(packet);

				continue;
			}

			/* Construct new full socket. */
			sock->new_fd = sys_socket(1);

			old_sock = sock;
			sock = get_socket_from_int(sock->new_fd);

			assert(sock);

			sock->parent = old_sock;
			sock->local_node = old_sock->local_node;
			sock->local_port = old_sock->local_port;
			sock->remote_port = src_port;
			sock->remote_node = src;

			socktable_put(node->tuple_table, sock, FULL_SOCKET);
		}

		/* If we're in the established state, perform primary communication; else, handshake*/
    if(tcpm_estab(sock->machine)) {
			nlog(MSG_LOG, "tcp_thread", "connection established! using sliding window protocol."); /* TODO */

    } else {
      /* Validate sequence numbers. */ //TODO use sentinel
      if(!tcpm_synsent(sock->machine) && !tcpm_firstrecv(sock->machine) && get_seqnum(ip_to_tcp(packet)) != sock->ack_num) {
			  nlog(MSG_WARNING, "tcp_thread", "Invalid sequence number!");
        /* TODO reset connection */
      }
    
      /* Handshake is a magical place where all ack numbers increase by one (so long as flags != ACK). */
      sock->ack_num = get_seqnum(ip_to_tcp(packet)) + (flags != TCP_FLAG_ACK);

		  nlog(MSG_LOG, "tcp_thread", "Socket not in established state; stepping state machine.");

      /* Step state machine (and perform needed action.) */
      if(tcpm_event(sock->machine, tcpm_packet_to_input(ip_to_tcp(packet)), NULL, NULL)) {
			  nlog(MSG_WARNING, "tcp_thread", "Invalid transition requested; fail!");
        /* Rely on error function; teardown socket? */
      }
    }

	}
}

/* query each interface and report when it goes down
 */
void *link_state_thread (void *arg) {
	ip_node_t *node = (ip_node_t*)arg;
	int i, nifs, link_state, age, timedout;

	nifs = van_node_nifs(node->van_node);

	nlog(MSG_LOG,"linkstate","Now monitoring the link state for %d interfaces...", nifs);

	while (1) {
		pthread_testcancel();
		sleep(1);

		for (i = 0; i < nifs; i++) {
			van_node_getifopt(node->van_node, i, VAN_IO_UPSTATE, (char*)&link_state, sizeof(int));
			update_link_line(i,link_state);
			//printf("it is now %d -- Last packet received on iface %d was %d seconds ago\n", time(NULL), i, time(NULL) - node->ifaces[i].age);
			pthread_mutex_lock(&node->ifaces[i].age_lock);
			age = node->ifaces[i].age;
			timedout = node->ifaces[i].timed_out;
			pthread_mutex_unlock(&node->ifaces[i].age_lock);

			if ((link_state) && !timedout && ((time(NULL) - age) > LINK_TIMEOUT)) {
				rtable_poison_iface(node->route_table, i);

				pthread_mutex_lock(&node->ifaces[i].age_lock);
				node->ifaces[i].timed_out = 1;
				pthread_mutex_unlock(&node->ifaces[i].age_lock);

				nlog(MSG_WARNING,"linkstate","Havn't heard anything on interface %d in over %d seconds.  Will now poison all routes that use this interface!", i, LINK_TIMEOUT);
				send_route_table(node);
				rtable_dump(node->route_table);
			}

			if (link_state == node->ifaces[i].cur_state) continue;

			if ((link_state) && (node->ifaces[i].cur_state == 0)) {
				nlog(MSG_LOG,"linkstate","[Node %d] The link on iface %d just came up!", node->van_node->vn_num, i);
				pthread_mutex_lock(&node->ifaces[i].lock);
				pthread_cond_signal(&node->ifaces[i].cond);
				pthread_mutex_unlock(&node->ifaces[i].lock);

				send_route_table(node);

			} else if ( (!link_state) && (node->ifaces[i].cur_state == 1)) {
				nlog(MSG_LOG,"linkstate","[Node %d] The link on iface %d just went down!", node->van_node->vn_num, i);
				rtable_poison_iface(node->route_table, i);


				send_route_table(node);

			}

			//node->ifaces[i].old_state = node->ifaces[i].cur_state;
			node->ifaces[i].cur_state = link_state;

			// if link is up but haven't recieved packets in LINK_TIMEOUT seconds, poison interface.
		}
	}

	return NULL;
}

/* rip_monitor
 *
 * this thread monitors a rip bqueue for incoming RIP packets
 */
void *rip_monitor (void *arg) {
	ip_node_t *node = (ip_node_t*)arg;

	int from;
	int iface;
	//rtable_entry_t *route;
	rtable_t *new_rtable;
	unsigned int total_length;
	rip_packet_t *rip;

	while (1) {
		pthread_cleanup_push((void(*)(void*))bqueue_poorly_implemented_cleanup, node->rip_q);
		bqueue_dequeue(node->rip_q, (void*)&rip); /* this will block */
		pthread_cleanup_pop(0);

		from = get_src(rip->packet);
		total_length = get_total_len(rip->packet);
		iface = rip->iface;

		//nlog(MSG_LOG,"rip","[Node %d] Processing an incoming RIP packet from interface %d",node->van_node->vn_num, iface);

		// do we have a route TO the node which just sent us a packet?
		if (update_route(node, from, iface, 1, from)) {
			nlog(MSG_LOG,"rip", "Due to an incoming packet, found a new route to %d on interface %d", from, iface);
			send_route_table(node);
		}

		//	printf("[Node %d]  New route added:  I can reach %d on iface %d\n", node->van_node->vn_num, from, iface);

		//	printf("\nA dump of the new route table:\n");
			rtable_dump(node->route_table);
		//	send_route_table(node);

		new_rtable = rtable_unserialize(rip->packet + HEADER_SIZE , total_length - HEADER_SIZE);

    /* If we've changed our route table, advertise. */
		if(rtable_merge(node->van_node->vn_num, node->route_table, new_rtable, lookup_route(node, from))) {
			nlog(MSG_LOG,"rip","It appears that rtable_merge has updated our local route table.  Broadcasting it to everyone");
      	send_route_table(node);
    	}
		

		//printf("\nA dump of the incoming route table:\n");
		//rtable_dump(new_rtable);

		//("\nA dump of our newly merged route table:\n");
		//rtable_dump(node->route_table);

		rtable_destroy(new_rtable);
		free(new_rtable);
		free(rip->packet);	
		free(rip);

	}

}

/* 
 * send our own route table to all peers
 */

void send_route_table(ip_node_t *node) {

	int rtable_size, nifs, i;
	nifs = van_node_nifs(node->van_node);
	int retval, packet_size;
	char *packet;
	char *rtable;
	vanaddr_t va;
	va.va_type = AT_P2P;

	for (i = 0; i < nifs; i++) {
		//printf("rip thread: start %d\n", i);
		if (get_if_state(node, i) == 0) continue;	

		// pack up our routing table.
		// if we know the peer on the other side, exclude the routes they gave us
		// in the serialized rtable we send

		//nlog(MSG_LOG,"send_route_table","Sending out a RIP packet on interface %d", i);

		rtable = rtable_serialize(node->route_table, &rtable_size, (node->ifaces[i].peer >= 0? node->ifaces[i].peer: -1) );

		packet_size = buildPacket(node, rtable, rtable_size, 0/*to*/, &packet, PROTO_RIP);
		//set_protocol(packet, PROTO_RIP);

		retval = van_node_send(node->van_node, i,packet, packet_size, 0, &va);

		free(packet);

		//nlog(MSG_LOG,"send_route_table","van_node_send returned %d", retval);
		//printf("[Node %d] Sent out dummy RIP packet out iface %d\n", node->van_node->vn_num, i);
		free (rtable); // don't call destory here because rtable is simply a char*
		//sleep(1);
	}
}

/* rip
 * 
 * this thread monitors the route table, and sends out any RIP
 * packets that are necessary
 */
void *rip (void*arg) {
	ip_node_t *node = (ip_node_t*)arg;

	sleep(4); // wait some time before sending our RIP packets
	nlog(MSG_LOG,"rip","rip thread started");

	while (1) {
		pthread_testcancel();
		sleep(RIP_SLEEP);
		send_route_table(node);

	}

}

/* sender
 * Sender thread.  Monitors the outgoing queue, and makes sures
 * that queued packets get sent
 */
void *sender (void *arg) {
	ip_node_t *this_node = (ip_node_t*)arg;
	ip_packet_t *data;
	vanaddr_t va;
	int retval, rval;

	nlog(MSG_LOG,"sender","Sendering thread started!");

	this_node->sending_q = malloc(sizeof(bqueue_t));
	bqueue_init(this_node->sending_q);

	while (1) {
		/* this will block */

		pthread_cleanup_push((void(*)(void*))bqueue_poorly_implemented_cleanup, this_node->sending_q);
		rval = bqueue_dequeue(this_node->sending_q, (void*)&data);
		pthread_cleanup_pop(0);

		if (rval == -EINVAL) {
			nlog(MSG_ERROR,"sender", "our queue disappeared!");
			exit(1);
		}
		nlog(MSG_LOG,"sender","Dequeued something...");
		va.va_type = data->addr_type;

		 retval = van_node_send(this_node->van_node, data->iface, data->packet, data->packet_size, 0, &va);
		 assert(data);
		 assert(data->packet);
		 free(data->packet);
		 free(data);
	}

	return NULL;
}

void listener_cleanup (void *buf) {
	//printf("Cleaning up on memory...\n");
	free(buf);
}

/* listener
 * Started in its own thread for each interface on this node
 */
void *listener (void *arg) {
	node_and_num_t *nnn = arg;
		
	int interface = nnn->iface;
	ip_node_t *node = nnn->node;

	int retval;
	char *buf;
	int mtu;
	const unsigned int me = node->van_node->vn_num;

	vanaddr_t addr;
	unsigned int dest;
	unsigned int src;
	unsigned int ttl;
	unsigned int proto; 
	uint16_t total_length;

	assert(node);

	van_node_getifopt(node->van_node, interface, VAN_IO_MTU, (char*)&mtu, sizeof(int));

	free(nnn);


	buf = malloc(mtu);

	pthread_cleanup_push((void(*)(void*))listener_cleanup,buf);

	nlog(MSG_LOG,"listener","Listener thread for node %d on interface %d ready.", node->van_node->vn_num, interface);

	while (1) {
		pthread_mutex_lock(&node->ifaces[interface].lock);
		while (get_if_state(node, interface) == 0) {
			/* wait to be signaled when this interface comes up */
			pthread_cond_wait(&node->ifaces[interface].cond, &node->ifaces[interface].lock);
		}
		pthread_mutex_unlock(&node->ifaces[interface].lock);

		//printf("while: listener\n");
		/* this will block */
		retval = van_node_recv( node->van_node, interface, buf, mtu, 0, &addr );
		
		if (retval >= 0) {
			if (retval > mtu) nlog(MSG_WARNING,"listener","I just got a packet on interface %d that has a size of %d, but this link has an MTU of %d !!!", interface, retval, mtu);
			//memcpy (&dest, buf+7, 1);
			//memcpy (&src, buf+6, 1);
			
			pthread_mutex_lock(&node->ifaces[interface].age_lock);
			node->ifaces[interface].age = time(NULL);
			node->ifaces[interface].timed_out = 0;
			pthread_mutex_unlock(&node->ifaces[interface].age_lock);

			total_length = get_total_len(buf);
			proto = get_protocol(buf);

			{
				/* Check packet checksum. */
				uint16_t packet_checksum, zero_checksum, calced_checksum;

				packet_checksum = get_checksum(buf);
				//nlog(MSG_LOG,"listener","Incoming packet.  Checksum field is: %d", packet_checksum);

				/* Clear checksum field. */
				//set_checksum(buf, 0);
				memset(buf+4,0,2);
				zero_checksum = get_checksum(buf);
				//nlog(MSG_LOG, "listener", "this should be zero: %d",zero_checksum);

				if((calced_checksum = ip_fast_csum((unsigned char*)buf, 2)) != packet_checksum) {
					nlog(MSG_ERROR,"listener", "Error: checksum mismatch.  ip_fast_csum returned %d, we think it should be %d", calced_checksum, packet_checksum);
					continue;

				} else {
					//nlog(MSG_LOG,"listener","Checksum match.");

					/* Restore checksum. */
					set_checksum(buf, packet_checksum);
				}
			}

			if (proto == PROTO_RIP) {
					
					char *packet = malloc(total_length);
					memcpy(packet, buf, total_length);

					//nlog(MSG_LOG,"rip","Just got a rip packet");
					rip_packet_t *rip = malloc(sizeof(rip_packet_t));
					
					rip->packet = packet;
					rip->iface = interface;

					pthread_cleanup_push((void(*)(void*))bqueue_poorly_implemented_cleanup, node->rip_q);
					bqueue_enqueue(node->rip_q,(void*)rip);
					pthread_cleanup_pop(0);

					continue;
			}

			dest = get_dst(buf);
			src = get_src(buf);
			ttl = get_ttl(buf);

			nlog(MSG_LOG,"listener","This IP packet is from %d, addressed to %d, has a ttl of %d, and a total length of %d",src, dest, ttl, total_length);
			if (total_length > mtu) { nlog(MSG_WARNING,"listener","WARNING: according to this IP packet header, the total length of the packet is %d, but this MTU for this link is only %d !!!   Will not forward.", total_length, mtu); continue; }

			if (dest != me) {
				rtable_entry_t *r = lookup_route(node, dest);
				char *packet = malloc(total_length); // will be free() when dequeed and sent

				memcpy(packet, buf, total_length);

				nlog(MSG_LOG,"listener","This packet is not addressed to me.  Will attempt to forward it out interface %d", r->iface);

				set_ttl(packet,ttl - 1 );
				/* now that we've altered the ttl, recompute the checksum */
				set_checksum(packet, 0); /* zero it out before recomputing */
				set_checksum(packet, ip_fast_csum((unsigned char*)packet, 2));

				ip_packet_t *p = malloc(sizeof(ip_packet_t));
				p->iface = r->iface;
				p->packet = packet;
				p->packet_size = total_length;
				p->addr_type = r->type;

				pthread_cleanup_push((void(*)(void*))bqueue_poorly_implemented_cleanup, node->sending_q);
				bqueue_enqueue(node->sending_q, (void*)p);
				pthread_cleanup_pop(0);

			} else {

				nlog(MSG_LOG,"listener","This packet is addressesed to me!");

				if (proto == PROTO_DATA) {
					char *data = malloc(total_length);
					nlog(MSG_LOG,"listener","Will pass up to the next layer");
					memcpy(data, buf, total_length);

					/* XXX is this really needed? enqueue will never really block*/
					pthread_cleanup_push((void(*)(void*))bqueue_poorly_implemented_cleanup, node->receiving_q);
					bqueue_enqueue(node->receiving_q, data);
					pthread_cleanup_pop(0);

				} else if (proto == PROTO_TCP) {
					/* give this to the TCP thread, so it can figure out what to do with it */
					nlog(MSG_LOG, "listener", "got a TCP packet!");
					char *packet = malloc(total_length); // will be free() when dequeed and sent

					memcpy(packet, buf, total_length);
					bqueue_enqueue(node->tcp_q, (void*)packet);

				} else {
					nlog(MSG_WARNING,"listener","I got a IP packet with an unknown payload type");
				}


			}

		} else {
			nlog(MSG_WARNING,"listener","van_node_recv returned %d", retval);
		}


	}
	pthread_cleanup_pop(1);
	return NULL;
}


/* van_driver_loaded
 * Dummy function.  Does nothing important
 */
int van_driver_loaded() {
	return 1;
}

/*
 * free all memory and join with all threads
 */
void van_driver_destory(ip_node_t *node) {
	int i;

	nlog(MSG_LOG,"shutdown","Attempting to cancel the sending thread... ");

	pthread_cancel(*node->sending_thread);
	pthread_join(*node->sending_thread, NULL);
	free(node->sending_thread);

	nlog(MSG_LOG,"shutdown","Attempting to cancel the listening threads... ");

	for (i = 0; i < van_node_nifs(node->van_node); i++) {
		pthread_cancel(*node->listening_thread[i]);
		pthread_join(*node->listening_thread[i], NULL);
		free(node->listening_thread[i]);
		nlog(MSG_LOG,"shutdown"," %d Done",i);
		pthread_cond_destroy(&node->ifaces[i].cond);
		pthread_mutex_destroy(&node->ifaces[i].lock);
		pthread_mutex_destroy(&node->ifaces[i].age_lock);
	}


	free(node->ifaces);

	nlog(MSG_LOG,"shutdown","Attempting to cancel the rip thread... ");
	pthread_cancel(*node->rip_thread);
	pthread_join(*node->rip_thread, NULL);
	free(node->rip_thread);
	//printf("Done.\n");

	nlog(MSG_LOG,"shutdown","Attempting to cancel the rip monitor thread... ");
	pthread_cancel(*node->rip_monitor_thread);
	pthread_join(*node->rip_monitor_thread, NULL);
	free(node->rip_monitor_thread);

	nlog(MSG_LOG,"shutdown","Attempting to cancel the link state thread... ");
	pthread_cancel(*node->link_state_thread);
	pthread_join(*node->link_state_thread, NULL);
	free(node->link_state_thread);

	nlog(MSG_LOG,"shutdown","Attempting to destroy the sending q... ");
	bqueue_destroy(node->sending_q);
	free(node->sending_q);

	nlog(MSG_LOG,"shutdown","Attempting to destroy the receiving q... ");
	bqueue_destroy(node->receiving_q);
	free(node->receiving_q);

	nlog(MSG_LOG,"shutdown","Attempting to destory the rip q... ");
	bqueue_destroy(node->rip_q);
	free(node->rip_q);

	nlog(MSG_LOG,"shutdown","Attempting to destory the route table... ");
	rtable_destroy(node->route_table);
	free(node->route_table);


	nlog(MSG_LOG,"shutdown"," -= Bye! =- ");
	return;
}


/* van_driver_init
 * Initilizes the van stuff, and whatever IP data structures
 * we create
 */
ip_node_t *van_driver_init(char *fname, int num) {
	van_node_t *vn = NULL;
	ip_node_t *node = malloc(sizeof(ip_node_t));
	int i, nifs;
	const int down = 0;
	node_and_num_t *nnn;

	if (van_init(fname)) {
		nlog(MSG_ERROR,"init", "Error: Can't init the van driver with network spec '%s'",fname);
		return NULL;
	}
	
	if ( !(vn = van_node_get(num))) {
		nlog(MSG_ERROR,"init", "Error Can't get node %d", num);
		return NULL;
	}

	assert(vn != NULL);
	node->van_node = vn;

	nifs = van_node_nifs(vn);
	node->ifaces = malloc(sizeof(iface_t) * nifs);
	nlog_set_menu("[node %d]  Initializing [=         ]",num );
	// start sending thread
	sleep(1);
	nlog(MSG_LOG, "init","starting sending thread");
	node->sending_thread = malloc(sizeof(pthread_t));
	pthread_create(node->sending_thread , 0 , sender, (void*)node);

	// start listening thread
	for (i = 0; i < nifs; i++) {
		nnn = malloc(sizeof(node_and_num_t));
		nnn->node = node;
		nnn->iface = i;
		node->listening_thread[i] = malloc(sizeof(pthread_t));
		pthread_create(node->listening_thread[i], 0, listener, (void*)nnn);

		//init ifaces array
		node->ifaces[i].cur_state = 0;
		node->ifaces[i].old_state = 0;
		node->ifaces[i].peer = -1;		
		node->ifaces[i].age = time(NULL);
		node->ifaces[i].timed_out = 0;

		// by default, set all links to be disabled.
		van_node_setifopt( vn, i, VAN_IO_UPSTATE, (char*)&down, sizeof(int));

		pthread_cond_init(&node->ifaces[i].cond, 0);
		pthread_mutex_init(&node->ifaces[i].lock, 0);
		pthread_mutex_init(&node->ifaces[i].age_lock, 0);


	}
	

	node->receiving_q = malloc(sizeof(bqueue_t));
	bqueue_init(node->receiving_q);

	node->rip_q = malloc(sizeof(bqueue_t));
	bqueue_init(node->rip_q);

	node->tcp_q = malloc(sizeof(bqueue_t));
	bqueue_init(node->tcp_q);

	// set up some static routes:
	node->route_table = rtable_new();

	nlog_set_menu("[node %d]  Initializing [==        ]",num );
	sleep(1);
	nlog_set_menu("[node %d]  Initializing [===       ]",num );
	//sleep(1);
	nlog_set_menu("[node %d]  Initializing [====      ]",num );
	//sleep(1);
	nlog_set_menu("[node %d]  Initializing [=====     ]",num );
	sleep(1);
	nlog_set_menu("[node %d]  Initializing [======    ]",num );
	//sleep(1);
	nlog_set_menu("[node %d]  Initializing [=======   ]",num );
	//sleep(1);
	nlog_set_menu("[node %d]  Initializing [========  ]",num );
	//sleep(1);
	nlog_set_menu("[node %d]  Initializing [========= ]",num );
	//sleep(1);
	nlog_set_menu("[node %d]  Initializing [==========]",num );
	sleep(1);

	// start up RIP threads
	
	node->link_state_thread = malloc(sizeof(pthread_t));
	pthread_create(node->link_state_thread, 0, link_state_thread, (void*)node);

	node->rip_monitor_thread = malloc(sizeof(pthread_t));
	pthread_create(node->rip_monitor_thread, 0, rip_monitor, (void*)node);

	node->rip_thread = malloc(sizeof(pthread_t));
	pthread_create(node->rip_thread, 0, rip, (void*)node);

	node->tcp_thread = malloc(sizeof(pthread_t));
	pthread_create(node->tcp_thread, 0, tcp_thread, (void*)node);

	node->tcp_send_thread = malloc(sizeof(pthread_t));
	pthread_create(node->tcp_send_thread, 0, tcp_send_thread, (void*)node);

	nlog (MSG_LOG,"init","Node %d running", vn->vn_num);	
	nlog_set_menu("[node %d]  1:Send data   2:Receive Data   3:Toggle Link State   q:Quit", vn->vn_num);
	// start sending thread
	
	// init TCP stufffs:
	v_tcp_init(node);

	return node;

}

/* buildPacket()
 * given some payload data, adds all the correct
 * IP headers.
 *
 * returns size of new packet
 */
int buildPacket(ip_node_t *node, char *data, int data_size, int to, char  **header, uint8_t proto) {

 	uint16_t total_length =8 + data_size;
	uint16_t ttl = 32;
	unsigned char src = node->van_node->vn_num;
	uint16_t dest = to;
	unsigned char start = 0;

	uint16_t checksum;

	int packet_size;

	// malloc ourselfs 8 bytes plus the size of the data
	packet_size = 8 + data_size;

	*header = malloc(packet_size);

	// zero everything.
	memset(*header,0,packet_size);

	// n=0 LSB
	start |= (1 << 7); // set ver = 0x1

	//memcpy(*header,&start,1);
	set_version(*header,1);
	set_protocol(*header,proto);

	//memcpy(*header + 1 ,&total_length, 2);
	set_total_len(*header, total_length);

	//memcpy(*header + 3, &ttl, 1);
	set_ttl(*header, ttl);

	//	 memcpy the src:
	// memcpy(*header+6, &src, 1);
	set_src(*header, src);

	// memcpy the dest:
	// memcpy(*header+7,&dest+0, 1);
	set_dst(*header, dest);

	// calculate the checksum:
	checksum = ip_fast_csum((unsigned char *)*header, 2);
	//nlog(MSG_LOG,"buildPacket", "checksum is %d", checksum);

	//memcpy(*header+4,&checksum,2);
	set_checksum(*header, checksum);
	checksum = 0;
	checksum = get_checksum(*header);
	//nlog(MSG_LOG, "buildPacket", "readback checksum is %d", checksum);

	//print_packet(*header,8);
	
	memcpy(*header+8, data, data_size);

	return packet_size;


}

int van_driver_sendto (ip_node_t *node, char *buf, int size, int to, uint8_t proto) {
	

	char *packet;
	int packet_size;

	// build packet

	if (to == node->van_node->vn_num) {
		// this packet is addressed to this local host
		nlog(MSG_LOG,"sendto","I was asked to send a packet to myself! Sending directly into local queue");
		char *data = malloc(size);	

		memcpy(data, buf, size);
		bqueue_enqueue(node->receiving_q, data);

		return -1;
	} else {
		vanaddr_t addr;
		addr.va_type = AT_P2P;
		rtable_entry_t *r;
		ip_packet_t *p = malloc(sizeof(ip_packet_t));
		int mtu;

		// queue up this packet
		r = rtable_get(node->route_table, to);
		if (!r) {
			nlog(MSG_WARNING,"sendto","I was asked to send a packet to node %d, but I have no route to that node", to);
			free(p);
			return -1;
		}
		
		van_node_getifopt(node->van_node, r->iface, VAN_IO_MTU, (char*)&mtu, sizeof(int));

		//uint16_t dport = get_destport(buf);
	//	nlog(MSG_LOG,"van_driver_sendto","pre buildPacket dport=%d",dport);
	//	nlog(MSG_LOG,"van_driver_sendto","about to call buildPacket.  size=%d", size);
		packet_size = buildPacket(node, buf, size, to, &packet, proto);

		//dport = get_destport(packet+8);
	//	nlog(MSG_LOG,"van_driver_sendto","get_destport=%d", dport);

		if (packet_size > mtu) {
			nlog(MSG_WARNING, "sendto", "WARNING Will not send this packet -- TOO BIG");
			free(packet);
			return -1;
		}

		nlog(MSG_LOG,"sendto","Attempting to send this packet (size %d) out along interface %d", packet_size, r->iface);
		
		p->iface = r->iface;
		p->packet = packet;
		p->packet_size = packet_size;
		p->addr_type = r->type;
		
		bqueue_enqueue(node->sending_q, (void*)p);

		// retval = van_node_send(node->van_node, r->iface, packet, packet_size, 0, &addr);
		// printf("Retval = %d\n", retval);
		// forward this along to another host
		//
	}
	
	return packet_size - HEADER_SIZE;
}

int van_driver_recvfrom (ip_node_t *node, char *buf, int size) {
	char *data;
	int data_size;

	// this will block;
	bqueue_dequeue(node->receiving_q, (void**)&data);
	data_size = get_total_len(data) - HEADER_SIZE ;
	nlog(MSG_LOG,"recvfrom","data_size=%d", data_size);
	nlog(MSG_LOG,"recvfrom","get_total_len=%d", get_total_len(data));

	if (size < data_size) {
		data_size = size;
	}

	//dump_bits(data, (data_size + 8)*8);	

	memcpy(buf, data+HEADER_SIZE, data_size);
	free(data);
	return data_size;
}


void entry_to_vaddr(rtable_entry_t* entry, vanaddr_t* vaddr) {
  vaddr->va_type = entry->type;
}

rtable_entry_t* lookup_route(ip_node_t *node, int addr) {
	rtable_entry_t *to_return =  rtable_get(node->route_table, addr);
	if (!to_return) { return NULL; }
	if (to_return->cost >= INFINITY) return NULL; else return to_return;
}

int update_route(ip_node_t *node, int addr, int iface, int cost, int next_hop) {
	int to_return;	
	rtable_entry_t *route = lookup_route(node, addr);
	if (!route) { add_route(node, addr, iface, cost, next_hop); return 1;}
	else {
		to_return = (route->iface != iface) || (route->cost != cost) || (route->next_hop != next_hop);
		route->iface = iface;
		route->cost = cost;
		route->next_hop = next_hop;
		return to_return;
	}

}

int add_route(ip_node_t *node, int addr, int iface, int cost, int next_hop) {
  rtable_entry_t* entry = malloc(sizeof(rtable_entry_t));
  vanaddr_t va;

  if(!entry) {
    return -1;
  }

  /* Obtain inteface type. */
  van_node_getifopt(node->van_node, iface, VAN_IO_IFTYPE, (char*) &va.va_type, sizeof(int));

  /* Populate routing table entry (table specific). */
  entry->addr = addr;
  entry->iface = iface;
  entry->type = va.va_type;
  entry->cost = cost;
  entry->next_hop = next_hop;

  /* Insert route into routing table. */
  rtable_put(node->route_table, entry);

  return 0;
}

void init_st(tcp_socket_t* thing, int ln, uint16_t lp, int rn, uint16_t rp) {
  thing->local_node = malloc(sizeof(ip_node_t));
  thing->local_node->van_node = malloc(sizeof(van_node_t));
  thing->local_node->van_node->vn_num = ln;

  thing->local_port = lp;
  thing->remote_node = rn;
  thing->remote_port = rp;
}

int main( int argc, char* argv[] ) {
    van_node_t *vn;
    int i = 0, in, len;
    char buf[ 256 ];
    vanaddr_t va;

    if( argc != 3 ) {
        printf("usage: %s node_num netconfig\n",argv[0]);
        return -1;
    }

    if( van_init( argv[ 2 ] ) ) {
        fprintf( stderr, "doh: van_init\n" );
        exit( 1 );
    }
    
    if( !( vn = van_node_get( atoi( argv[ 1 ] ) ) ) ) {
        fprintf( stderr, "doh: van_node_get\n" );
        exit( 1 );
    }

    printf( "Node %i all set [ CTL-D to exit ]\n", atoi( argv[ 1 ] ) );

    while( i != 9 ) {
		int err;
	    printf( "Command: " );

		fflush(stdout);
		memset(buf,0,80);
		err = read(STDIN_FILENO, buf, 80);
		if( err <= 0 ) { printf("\n"); break; }
		buf[79] = 0;

		sscanf(buf, "%d", &i);

	    switch( i ) {
	    case 0:
            /* read */

            printf( "Interface: " );
			fflush(stdout);
			memset(buf,0,80);
			read(STDIN_FILENO, buf, 80);
			buf[79] = 0;

			sscanf(buf, "%d", &in);

            if( ( len = van_node_recv( vn, in, buf, 255, 0, &va ) ) > 0 ) {
                buf[ len ] = 0;
                printf( "-- Received --\n"
                        "  Interface: %d\n"
                        "  Type: %s\n"
                        "  Size: %d\n"
                        "  Data: %s\n",
                        in, types[ va.va_type ], len, buf );
            }
            else
        		printf( "van_recv_error (%d): %s\n", len, strerror( -len ) );

            break;

        case 1:
            /* write */

            printf( "Interface: " );
            scanf( "%d", &in );

            /* make sure we're sending to an address of the
             * proper type */
            van_node_getifopt( vn, in, VAN_IO_IFTYPE, (char*) &va.va_type,
                sizeof( int ) );

            printf( "Message: " );
            scanf( "%s", buf );
			len = van_node_send( vn, in, buf, strlen( buf ), 0, &va );
			if( len > 0 )
                printf( "send success\n" );
            else
        		printf( "van_send_error (%d): %s\n", len, strerror( -len ) );

            break;
        }
    }

    /* van_destroy is implicitly called here via an at_exit() handler ...
     * this also takes care of van_node_put()'ing all relevant van_node_t's */

    return 0;
}
