#ifndef KERNEL_FUNC_H
#define KERNEL_FUNC_H

#include "pcb.h"
#include "pcb_list.h"
#include "signals.h"
#include "kernel.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>

extern int max_pid;
extern ucontext_t scheduler_context;

struct pcb *k_process_create(struct pcb *parent);

void k_process_kill(struct pcb *process, int signal);

void k_process_cleanup(struct pcb *process);

void k_process_cleanup_1(struct pcb *process, char *command);

void k_process_kill(pcb *process, int signal);

void wake_up_parent(pcb *process);

#endif
