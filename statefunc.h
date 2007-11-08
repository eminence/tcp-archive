#ifndef __STATEFUNC_H_
#define __STATEFUNC_H_

#include "state.h"

int send_packet_with_flags (sid_t prev, sid_t next, void* context, void *arg, void *tran_arg);


#endif
