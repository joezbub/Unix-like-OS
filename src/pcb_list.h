#ifndef PCB_LIST_H
#define PCB_LIST_H

#include "pcb.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/types.h>
#include <ucontext.h>

typedef struct pcb_list_node
{
    struct pcb_list_node *next;
    struct pcb *pcb;
} pcb_list_node;

struct pcb_list_node *add_pcb(struct pcb_list_node *head, struct pcb *pcb);

struct pcb *get_pcb_from_pid(struct pcb_list_node *head, int pid);

struct pcb_list_node *remove_pcb_from_pid(struct pcb_list_node *head, int pid);

pcb_list_node *soft_remove(pcb_list_node *head, int pid);

#endif