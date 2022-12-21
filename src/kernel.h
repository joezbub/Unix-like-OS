#ifndef KERNEL_H
#define KERNEL_H

#include <ucontext.h>
#include "pcb.h"

extern struct pcb_list_node *queues[3];
extern struct pcb_list_node *head;
extern int ticks;
extern pid_t terminal_control;

#endif // !KERNEL
