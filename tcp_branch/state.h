#ifndef STATE_H
#define STATE_H

#include <htable.h>
#include <bqueue.h>
#include <stdint.h>

#define MAX_FANOUT  16
#define RESTART_OK		0x01	// Restart was not due to a problem
#define RESTART_ABORT	0x02	// Restart was due to a problem (RST,...)
#define RESTART_INIT		0x04	// Restart was actually a start (i.e., first time)

/*
 * Data types.
 */

typedef uint32_t sid_t;
typedef uint32_t input_t;

typedef struct {
  uint8_t mark;
  sid_t id;
  htable_t transitions;
  void* argd;
  void (*action)(sid_t id, void* context, void* argd, void* args);
  void (*error)(sid_t id, void* context, void* args);
} state_t;

typedef struct {
 int (*action)(sid_t prev, sid_t next, void* context, void* argd, void* argt);
 void* argd;
 state_t* next;
} transition_t;

typedef struct {
  void* context;
  state_t* current;
  state_t* prev;
  state_t* root;
} machine_t;

/*
 * Operations.
 */

machine_t* machine_new(state_t* start, void* context);
void machine_destroy(machine_t* machine);
void machine_reset(machine_t* machine, uint8_t reason);
state_t* machine_step(machine_t* machine, input_t input, void* argt, void* args);

state_t* state_new(sid_t id, void (*action)(sid_t, void*, void*, void*), void (*error)(sid_t, void*, void*), void* argd);
void state_destroy(state_t* state);
int state_transition(state_t* state, state_t* next, input_t input, int (*action)(sid_t, sid_t, void*, void*, void*), void* argd);

/*
 * Macros.
 */

#define machine_current(mptr)   ((mptr)->current)
#define machine_context(mptr)   ((mptr)->context)
#define state_id(sptr)          ((sptr)->id)

#endif
