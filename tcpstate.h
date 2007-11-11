#ifndef TCPSTATE_H
#define TCPSTATE_H

#include "state.h"
#include "statefunc.h"
#include "tcp.h"

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
  ON_RECV_FIN,
  ON_RECV_RST,
  ON_RECV_SYN_ACK,
  ON_RECV_FIN_ACK,
  ON_INVALID,
} tinput_t;

typedef struct tcp_machine__ {
  machine_t* sm;
} tcp_machine_t;

/* Forward declaration. */
struct tcp_socket__;

/* Core functionality. */
tcp_machine_t* tcpm_new(struct tcp_socket__* context, uint8_t clone);
void tcpm_destroy(tcp_machine_t* machine);
int tcpm_event(tcp_machine_t* machine, tinput_t event, void* argt, void* args);
int tcpm_packet_to_input(const char* packet);
const char* tcpm_strstate(int state);

#define tcpm_state(m)       ((m)->sm->current->id)
#define tcpm_canbind(m)     (tcpm_state(m) == ST_CLOSED)
#define tcpm_estab(m)       (tcpm_state(m) == ST_ESTAB)
#define tcpm_firstrecv(m)   (tcpm_state(m) == ST_LISTEN)
#define tcpm_synsent(m)     (tcpm_state(m) == ST_SYN_SENT)

#endif
