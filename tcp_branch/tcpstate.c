#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tcpstate.h"
#include "state.h"
#include "statefunc.h"
#include "ippacket.h"
#include "tcppacket.h"
#include "fancy_display.h"
#include "notify.h"

void* alloc_byte(uint8_t ibyte) {
  void* byte = malloc(1);

  /* Ensure success. */
  assert(byte);

  /* Set contents. */
  *((uint8_t*)byte) = ibyte;

  return byte;
}

/* Core functionality. */
tcp_machine_t* tcpm_new(tcp_socket_t* context, uint8_t clone) {
  tcp_machine_t *machine        = malloc(sizeof(tcp_machine_t));
  state_t       *st_closed      = state_new(ST_CLOSED,      in_closed,		fail_with_reset,  NULL), // was null (error func)
                *st_syn_sent    = state_new(ST_SYN_SENT,    NULL, 			fail_with_reset,  NULL),
                *st_syn_rcvd    = state_new(ST_SYN_RCVD,    NULL, 			fail_with_reset,  NULL),
                *st_listen      = state_new(ST_LISTEN,      NULL, 			fail_with_reset,  NULL), // was null
                *st_estab       = state_new(ST_ESTAB,       in_estab,		fail_with_reset,  NULL),
                *st_fin_wait_1  = state_new(ST_FIN_WAIT1,   NULL, 			fail_with_reset,  NULL), // was null
                *st_fin_wait_2  = state_new(ST_FIN_WAIT2,   NULL, 			fail_with_reset,  NULL), // was null
                *st_time_wait   = state_new(ST_TIME_WAIT,   in_timewait,	fail_with_reset,  NULL), // was null
                *st_closing     = state_new(ST_CLOSING,     NULL, 			fail_with_reset,  NULL), // was null
                *st_close_wait  = state_new(ST_CLOSE_WAIT,  in_closewait, 	fail_with_reset,  NULL), // was null
                *st_last_ack    = state_new(ST_LAST_ACK,    NULL, 			fail_with_reset,  NULL); // was null
  
  /* Initialize state machine. */
  machine->sm = machine_new(st_closed, context);

  /* We cloned this machine; thus, jump into Listen state. */
  if(clone) {
    machine->sm->current = st_listen;
  }

  /* Out of memory: just explode. */
  assert(st_closed);
  assert(st_syn_sent);
  assert(st_syn_rcvd);
  assert(st_listen);
  assert(st_estab);
  assert(st_fin_wait_1);
  assert(st_fin_wait_2);
  assert(st_time_wait);
  assert(st_closing);
  assert(st_close_wait);
  assert(st_last_ack);
  assert(machine);
  assert(machine->sm);
  
  /* Add transitions. */
  assert(0 == state_transition(st_closed,      st_syn_sent,    ON_ACTIVE_OPEN,     do_send_flags,          alloc_byte(TCP_FLAG_SYN)));
  assert(0 == state_transition(st_closed,      st_listen,      ON_PASSIVE_OPEN,    do_listen,              NULL)); /* XXX init state. */
  assert(0 == state_transition(st_syn_sent,    st_closed,      ON_CLOSE,           NULL,                   NULL)); /* XXX free state. */
  assert(0 == state_transition(st_syn_sent,    st_syn_rcvd,    ON_RECV_SYN,        do_send_flags,          alloc_byte(TCP_FLAG_ACK)));
  assert(0 == state_transition(st_syn_sent,    st_estab,       ON_RECV_SYN_ACK,    do_connect,             alloc_byte(TCP_FLAG_ACK)));
  assert(0 == state_transition(st_listen,      st_closed,      ON_CLOSE,           do_close,               alloc_byte(CLOSE_OK))); /* notify close call THAT CLOSED in listen state. */
  assert(0 == state_transition(st_listen,      st_syn_rcvd,    ON_RECV_SYN,        do_send_flags,          alloc_byte(TCP_FLAG_SYN | TCP_FLAG_ACK)));
  assert(0 == state_transition(st_syn_rcvd,    st_estab,       ON_RECV_ACK,        NULL,                   NULL));
  assert(0 == state_transition(st_estab,       st_fin_wait_1,  ON_CLOSE,           do_send_flags,          alloc_byte(TCP_FLAG_FIN)));
  assert(0 == state_transition(st_estab,       st_close_wait,  ON_RECV_FIN,        do_send_flags,          alloc_byte(TCP_FLAG_ACK))); /* XXX XXX do_recv_tcp DOESN'T sends ack anymore. XXX XXX*/
  assert(0 == state_transition(st_estab,       st_estab,       ON_NONE,            NULL,                   NULL)); // can recv data -- noop
  assert(0 == state_transition(st_estab,       st_estab,       ON_RECV_ACK,        NULL,                   NULL)); // can ack/send data  -- noop
  assert(0 == state_transition(st_fin_wait_1,  st_closing,     ON_RECV_FIN,        do_send_flags,          alloc_byte(TCP_FLAG_ACK)));
  assert(0 == state_transition(st_fin_wait_1,  st_time_wait,   ON_RECV_FIN_ACK,    do_send_flags,          alloc_byte(TCP_FLAG_ACK)));
  assert(0 == state_transition(st_fin_wait_1,  st_fin_wait_2,  ON_RECV_ACK,        do_close,               alloc_byte(CLOSE_OK))); /* first close, initial notify; close succeeds, can still recv. */
  assert(0 == state_transition(st_fin_wait_1,  st_fin_wait_1,  ON_NONE,            NULL,                   NULL)); // can recv data -- noop
  assert(0 == state_transition(st_fin_wait_2,  st_time_wait,   ON_RECV_FIN,        do_send_flags,          alloc_byte(TCP_FLAG_ACK)));
  assert(0 == state_transition(st_fin_wait_2,  st_fin_wait_2,  ON_NONE,            NULL,                   NULL)); // can recv data -- noop
  assert(0 == state_transition(st_close_wait,  st_last_ack,    ON_CLOSE,           do_send_flags,          alloc_byte(TCP_FLAG_FIN)));
  assert(0 == state_transition(st_close_wait,  st_close_wait,  ON_RECV_ACK,        NULL,                   NULL)); // can ack/send data  -- noop
  assert(0 == state_transition(st_closing,     st_time_wait,   ON_RECV_ACK,        NULL,                   NULL));
  assert(0 == state_transition(st_time_wait,   st_closed,      ON_TIMEOUT,         do_close,               alloc_byte(CLOSE_EOF))); /* first close, final notify: next read fails. */ 
  assert(0 == state_transition(st_last_ack,    st_closed,		ON_RECV_ACK,        do_close,					  alloc_byte(CLOSE_OK))); /* second close: just returns OK and socket is fully closed. */

  return machine;
}

void tcpm_destroy(tcp_machine_t* machine) {
  machine_destroy(machine->sm);
}

void tcpm_reset(tcp_machine_t* machine) {
	machine_reset(machine->sm, RESTART_ABORT);
}

int tcpm_event(tcp_machine_t* machine, tinput_t event, void* argt, void* args) {
  int result = (NULL == machine_step(machine->sm, event, argt, args));

  update_tcp_table(machine->sm->context);
  nlog(MSG_LOG, "tcpm_event", "Socket %d now in state %s", ((tcp_socket_t*)machine->sm->context)->fd, tcpm_strstate(machine->sm->current->id));

  return result;
}

int tcpm_packet_to_input(const char* packet) {
  /* Directly convert each symbol. */
  switch(get_flags(packet)) {
    case TCP_FLAG_ACK:
      return ON_RECV_ACK;
    case TCP_FLAG_SYN:
      return ON_RECV_SYN;
    case TCP_FLAG_FIN:
      return ON_RECV_FIN;
    case TCP_FLAG_RST:
      return ON_RECV_RST;
    case TCP_FLAG_SYN | TCP_FLAG_ACK:
      return ON_RECV_SYN_ACK;
    case TCP_FLAG_FIN | TCP_FLAG_ACK:
      return ON_RECV_FIN_ACK;
     case 0: // NO FLAGS
      return ON_NONE;
    default:
      nlog(MSG_ERROR, "tcpm_packet_to_input", "invalid flags: %d", get_flags(packet));
      return ON_INVALID;
  }
}

const char* tcpm_strstate(int state) {
  switch(state) {
    case ST_CLOSED:
      return "Closed";

    case ST_SYN_SENT:
      return "SYN sent";

    case ST_SYN_RCVD:
      return "SYN received";

    case ST_LISTEN:
      return "Listen";

    case ST_ESTAB:
      return "Established";

    case ST_FIN_WAIT1:
      return "FIN wait (1)";

    case ST_FIN_WAIT2:
      return "FIN wait (2)";

    case ST_TIME_WAIT:
      return "Time wait";

    case ST_CLOSING:
      return "Closing";

    case ST_CLOSE_WAIT:
      return "Close wait";

    case ST_LAST_ACK:
      return "Last ACK";

    default:
      return "Invalid State";
  }
}
