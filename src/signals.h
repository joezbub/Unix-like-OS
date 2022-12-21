#ifndef SIGNALS_H
#define SIGNALS_H

#include "pcb.h"
#include <ucontext.h>
#include "kernel.h"

#define S_SIGSTOP 1
#define S_SIGCONT 2
#define S_SIGTERM 3

extern struct pcb *active_pcb;
extern ucontext_t scheduler_context;

void sigalrm_handler(int signo);
void sigstop_handler(int signo);
void sigint_handler(int signo);

#endif
