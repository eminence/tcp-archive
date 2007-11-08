#include <assert.h>

#include "tcpstate.h"
#include "state.h"

/* Core functionality. */
tcp_machine_t* tcpm_new(tcp_socket_t* context) {
  tcp_machine_t *machine        = malloc(sizeof(tcp_machine_t));
                *st_closed      = state_new(ST_CLOSED,      NULL, NULL, NULL),
                *st_syn_sent    = state_new(ST_SYN_SENT,    NULL, NULL, NULL),
                *st_syn_rcvd    = state_new(ST_SYN_RCVD,    NULL, NULL, NULL),
                *st_listen      = state_new(ST_LISTEN,      NULL, NULL, NULL),
                *st_estab       = state_new(ST_ESTAB,       NULL, NULL, NULL),
                *st_fin_wait1   = state_new(ST_FIN_WAIT1,   NULL, NULL, NULL),
                *st_fin_wait2   = state_new(ST_FIN_WAIT2,   NULL, NULL, NULL),
                *st_time_wait   = state_new(ST_TIME_WAIT,   NULL, NULL, NULL),
                *st_closing     = state_new(ST_CLOSING,     NULL, NULL, NULL),
                *st_close_wait  = state_new(ST_CLOSE_WAIT,  NULL, NULL, NULL),
                *st_last_ack    = state_new(ST_LAST_ACK,    NULL, NULL, NULL);
  
  /* Initialize state machine. */
  machine->sm = machine_new(st_closed, context);

  /* Out of memory: just explode. */
  assert(st_closed);
  assert(st_syn_sent);
  assert(st_syn_rcvd);
  assert(st_listen);
  assert(st_estab);
  assert(st_fin_wait1);
  assert(st_fin_wait2);
  assert(st_time_wait);
  assert(st_closing);
  assert(st_close_wait);
  assert(st_last_ack);
  assert(machine);
  assert(machine->sm);
  
  /* Add transitions. */
  assert(state_transition(st_closed,      st_syn_sent,    ON_ACTIVE_OPEN,     send_packet_with_flags, NULL));
  assert(state_transition(st_closed,      st_listen,      ON_PASSIVE_OPEN,    send_packet_with_flags, NULL));
  assert(state_transition(st_syn_sent,    st_closed,      ON_CLOSE,           send_packet_with_flags, NULL));
  assert(state_transition(st_syn_sent,    st_syn_rcvd,    ON_RECV_SYN,        send_packet_with_flags, NULL));
  assert(state_transition(st_syn_sent,    st_estab,       ON_RECV_SYN_ACK,    send_packet_with_flags, NULL));
  assert(state_transition(st_listen,      st_closed,      ON_CLOSE,           send_packet_with_flags, NULL));
  assert(state_transition(st_listen,      st_syn_rcvd,    ON_RECV_SYN,        send_packet_with_flags, NULL));
  assert(state_transition(st_syn_rcvd,    st_estab,       ON_RECV_ACK,        send_packet_with_flags, NULL));
  assert(state_transition(st_estab,       st_fin_wait_1,  ON_CLOSE,           send_packet_with_flags, NULL));
  assert(state_transition(st_estab,       st_close_wait,  ON_RECV_FIN,        send_packet_with_flags, NULL));
  assert(state_transition(st_fin_wait_1,  st_closing,     ON_RECV_FIN,        send_packet_with_flags, NULL));
  assert(state_transition(st_fin_wait_1,  st_time_wait,   ON_RECV_FIN_ACK,    send_packet_with_flags, NULL));
  assert(state_transition(st_fin_wait_1,  st_fin_wait_2,  ON_RECV_ACK,        send_packet_with_flags, NULL));
  assert(state_transition(st_close_wait,  st_last_ack,    ON_CLOSE,           send_packet_with_flags, NULL));
  assert(state_transition(st_fin_wait_2,  st_time_wait,   ON_RECV_FIN,        send_packet_with_flags, NULL));
  assert(state_transition(st_closing,     st_time_wait,   ON_RECV_ACK,        send_packet_with_flags, NULL));
  assert(state_transition(st_time_wait,   st_closed,      ON_TIMEOUT,         send_packet_with_flags, NULL));
}

void tcpm_destroy(tcp_machine_t* machine) {
  machine_destroy(machine->sm);
}

int tcpm_event(tcp_machine_t* machine, tinput_t event, void* argt, void* args) {
  return NULL == machine_step(machine->sm, event, argt, args);
}
