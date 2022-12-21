#include <stdio.h>
#include "sched.h"
#include <time.h>
#include <stdlib.h>
#include "kernel.h"
#include "pcb_list.h"
#include "user_func.h"
#include "kernel_func.h"
#include <string.h>
#include "log.h"

struct pcb_list_node *queues[] = {NULL, NULL, NULL};

void handleFinish()
{
    pcb *parent_pcb = active_pcb->parent;
    if (parent_pcb == NULL || get_pcb_from_pid(head, parent_pcb->pid) == NULL)
    {
        p_exit();
    }
    else
    {
        wake_up_parent(active_pcb);
        active_pcb->term_status = TERM_NORMAL;
        active_pcb->updated_flag = 1;
        active_pcb->status = ZOMBIED;
        log_command(ZOMBIE, active_pcb, 0);
        queues[active_pcb->priority + 1] = soft_remove(queues[active_pcb->priority + 1], active_pcb->pid);
    }
}

void unblock()
{
    pcb_list_node *curr = head;
    while (curr != NULL)
    {
        if (curr->pcb->status == BLOCKED && curr->pcb->sleeping && curr->pcb->blocked_until <= ticks)
        {
            curr->pcb->status = RUNNING;
            queues[curr->pcb->priority + 1] = add_pcb(queues[curr->pcb->priority + 1], curr->pcb);
            curr->pcb->sleeping = false;
            log_command(UNBLOCKED, curr->pcb, 0);
        }
        curr = curr->next;
    }
}

void schedule(void)
{
    unblock();
    if (active_pcb != NULL)
    {
        if (active_pcb->time_expired)
        {
            queues[active_pcb->priority + 1] = add_pcb(queues[active_pcb->priority + 1], active_pcb);
        }
        else if (active_pcb->pid != 0)
        {
            // handle active pcb finishing
            handleFinish();
        }
    }

    if (queues[0] == NULL && queues[1] == NULL && queues[2] == NULL)
    {
        ticks += 1;
        // Queues empty, so run the idle process
        // setcontext(&main_context);
        pcb idle;
        idle.name = "Idle process";
        idle.priority = 0;
        idle.pid = -1;
        log_command(SCHEDULE, &idle, 0);
        active_pcb = NULL;
        setcontext(&idle_context);
    }

    // Keep trying while the queue of pointed at by turn is not empty
    int queue_number = 0;

    // printf("Random start\n");
    while (1)
    {
        int turn = rand() % 190;
        // printf("turn is: %d\n", turn);
        if (turn < 90)
        {
            if (queues[0] == NULL)
                continue;
            queue_number = 0;
            break;
        }
        else if (turn < 150)
        {
            if (queues[1] == NULL)
                continue;
            queue_number = 1;
            break;
        }
        else
        {
            if (queues[2] == NULL)
                continue;
            queue_number = 2;
            break;
        }
    }

    // pcb_list_node *node = queues[queue_number];
    // while (node != NULL)
    // {
    //     printf("----  %d -----\n", node->pcb->status);
    //     node = node->next;
    // }

    // printf("Queue number: %d\n", queue_number);
    active_pcb = queues[queue_number]->pcb;
    queues[queue_number] = remove_pcb_from_pid(queues[queue_number], queues[queue_number]->pcb->pid);

    log_command(SCHEDULE, active_pcb, 0);

    ticks += 1;

    // printf("to run: %s\n", active_pcb->name);
    active_pcb->time_expired = 0;
    setcontext(active_pcb->context);
}