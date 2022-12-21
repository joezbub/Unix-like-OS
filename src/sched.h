#ifndef SCHED_H
#define SCHED_H

#include "pcb.h"
#include <ucontext.h>

extern struct pcb *active_pcb;
extern struct ucontext_t main_context;
extern struct ucontext_t idle_context;

void schedule(void);

#endif // !SCHED
