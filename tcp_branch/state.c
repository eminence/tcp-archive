#include <stdio.h>
#include <stdlib.h>
#include <bqueue.h>

#include "state.h"
#include "fancy_display.h"

machine_t* machine_new(state_t* start, void* context) {
  machine_t *machine;

  /* Out of memory. */
  if(!(machine = malloc(sizeof(machine_t)))) {
  	 assert(0 && "Out of memory.");
    return NULL;
  }
  
  machine->context = context;
  machine->root = start;

  // set up initial state
  machine_reset(machine, RESTART_INIT);

  return machine;
}

void machine_destroy(machine_t* machine) {
  /* Avoid destroying null-pointer. */
  if(!machine) {
    return;
  }

  /* Cleanup machine, starting at root. */
  if(machine->root) {
    state_destroy(machine->root);
  }
  
  /* Take ownership of context structure. */
  if(machine->context) {
    free(machine->context);
  }

  free(machine);
}

void machine_reset(machine_t* machine, uint8_t reason) {
	machine->current = machine->root;
	machine->prev = machine->root;
	
	/* Invoke state action, if provided. */
  	if(machine->current->action) {
   	machine->current->action(machine->current->id, machine->context, machine->current->argd, (void*)reason);
  	}
}

state_t* machine_step(machine_t* machine, input_t input, void* argt, void* args) {
  transition_t* tr;

  /* Invalid machine. */
  if(!machine->current) {
    nlog(MSG_ERROR, "machine_step", "machine not defined");
    return NULL;
  }

  /* Invalid transition. */
  if(!(tr = htable_get(&machine->current->transitions, input))) {
    /* Invoke error function, if provided. */
    if(machine->current->error) {
      machine->current->error(machine->current->id, machine->context, args);
    }

    nlog(MSG_ERROR, "machine_step", "invalid input (%d); returning to root state.", input);
	 machine_reset(machine, RESTART_ABORT);

    return NULL;
  }

  /* Invoke transition action, if provided. */
  if(tr->action) {
    /* Transition function must return 0; else, failure. */
    if(tr->action(machine->current->id, tr->next->id, machine->context, tr->argd, argt)) {
      /* Invoke error function, if provided. */
      if(machine->current->error) {
        machine->current->error(machine->current->id, machine->context, args);
      }
    
      nlog(MSG_ERROR, "machine_step", "Transition function returned 0; returning to root state");
		machine_reset(machine, RESTART_ABORT);

      return NULL;
    }
  }

  nlog(MSG_LOG, "machine_step", "Input %d: step from %d to %d", input, machine->current->id, tr->next->id);

  /* Update machine state. */
  machine->prev = machine->current;
  machine->current = tr->next;

  /* Invoke state action, if provided. */
  if(machine->current->action) {
  	 // IF we are returning to the root state, do not pass state argument; instead, notify that restart is occuring naturally.
    machine->current->action(machine->current->id, machine->context, machine->current->argd, machine->current == machine->root ? RESTART_OK : args);
  }
 
  /* Alert GUI of transition. */

  return machine->current;
}

state_t* state_new(sid_t id, void (*action)(sid_t, void*, void*, void*), void (*error)(sid_t, void*, void*), void* argd) {
  state_t* state;

  /* Out of memory. */
  if(!(state = malloc(sizeof(state_t)))) {
    return NULL;
  }

  /* Initialize transition table. */
  htable_init(&state->transitions, MAX_FANOUT);

  state->id = id;
  state->action = action;
  state->error = error;
  state->argd = argd;
  state->mark = 0;

  return state;
}

void state_destroy(state_t* state) {
  transition_t* tr;
  state_t* marked;
  bqueue_t garbage;

  /* Avoid destroying null-pointer. */
  if(!state) {
    return;
  }

  /* Set up garbage heap. */
  bqueue_init(&garbage);
  
  /* Mark state as processed. */
  state->mark = 1;

  /* Recursively destroy all linked-to states; utilize "mark and sweep". */
  htable_iterate_begin(&state->transitions, tr, transition_t) {
    /* If not yet queued, flag for obliteration. */
    if(!tr->next->mark) {
      tr->next->mark = 1; 
      bqueue_enqueue(&garbage, tr->next);
    }

    /* Free transition. */
    free(tr->argd);
    free(tr);
  } htable_iterate_end();

  /* After marking, sweep. */
  while(!bqueue_trydequeue(&garbage, (void**)&marked)) {
    state_destroy(marked);
  }

  htable_destroy(&state->transitions);
  free(state->argd);
  free(state);

  /* Destroy garbage heap. */
  bqueue_destroy(&garbage);
}

int state_transition(state_t* state, state_t* next, input_t input, int (*action)(sid_t, sid_t, void*, void*, void*), void* argd) {
  transition_t* tr;

  /* Both state and next must not be NULL. */
  if(!state || !next) {
    return -1;
  }

  /* Ensure transition not already assigned. */
  if(htable_get(&state->transitions, input)) {
    return -2;
  }

  /* Out of memory. */
  if(!(tr = malloc(sizeof(transition_t)))) {
    return -3;
  }

  tr->next = next;
  tr->action = action;
  tr->argd = argd;

  /* Insert transition. */
  htable_put(&state->transitions, input, tr);

  return 0;
}
