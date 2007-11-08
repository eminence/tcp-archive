#ifndef TCPSTATE_H
#define TCPSTATE_H

#include "tcp.h"
#include "state.h"

/* TCP state constants. */
typedef enum {
  ST_CLOSED,
  ST_SYN_SENT,
  ST_SYN_RCVD,
  ST_LISTEN,
  ST_ESTAB,
  ST_FIN_WAIT1,
  ST_FIN_WAIT2,
  ST_TIME_WAIT,
  ST_CLOSING,
  ST_CLOSE_WAIT,
  ST_LAST_ACK
} tsid_t;

/* TCP transition constants. */
typedef enum {
  ON_ACTIVE_OPEN,
  ON_PASSIVE_OPEN,
  ON_CLOSE,
  ON_TIMEOUT,
  ON_RECV_ACK,
  ON_RECV_SYN,
  ON_RECV_SYN_ACK,
  ON_RECV_FIN,
  ON_RECV_FIN_ACK
} tinput_t;

struct {
  machine_t* sm;
} tcp_machine_t;

/* Core functionality. */
tcp_machine_t* tcpm_new(tcp_socket_t* context);
void tcpm_destroy(tcp_machine_t* machine);
int tcpm_event(tcp_machine_t* machine, tinput_t event);

#define tcpm_state(m) ((m)->sm->current->id)
#define flags_to_input(f) NULL

#endif
