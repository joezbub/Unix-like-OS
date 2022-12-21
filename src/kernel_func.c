#include "pcb.h"
#include "pcb_list.h"
#include "signals.h"
#include "kernel_func.h"
#include <signal.h>
#include <stdio.h>
#include "log.h"
#include "kernel.h"
#include <stdlib.h>
#include <sys/types.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>
#include "string.h"

/**
 * NAME          k_process_create
 * DESCRIPTION   Takes in parent pcb and creates and returns child pcb. Updates parent child pids as well.
 * FOUND IN      kernel_func.c
 * RETURNS       child process pcb.
 * ERRORS        throws no errors.
 */
struct pcb *k_process_create(struct pcb *parent)
{
    // Initialize child context
    struct pcb *child = (struct pcb *)malloc(sizeof(struct pcb));
    child->context = (struct ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(child->context);
    sigemptyset(&child->context->uc_sigmask);
    child->context->uc_link = &scheduler_context;

    void *sp = malloc(SIGSTKSZ);
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);
    child->context->uc_stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};

    // Initialize other fields
    child->parent = parent;
    child->pid = max_pid++;
    child->num_children = 0;
    child->num_zombies = 0;
    child->num_to_wait_for = 0;
    child->fds[0] = 0;
    child->fds[1] = 1;
    child->status = RUNNING;
    child->updated_flag = 0;
    child->priority = 0;
    child->sleeping = false;

    // Add child pid to parent child pid array
    pid_t *new_child_pids = malloc((parent->num_children + 1) * sizeof(pid_t));
    for (int i = 0; i < parent->num_children; ++i)
    {
        new_child_pids[i] = parent->child_pids[i];
    }
    new_child_pids[parent->num_children] = child->pid;
    free(parent->child_pids);
    parent->num_children++;
    parent->child_pids = new_child_pids;

    return child;
}

// // currently inactive
// void k_process_kill_1(struct pcb *process, int signal)
// {
//     switch (signal)
//     {
//     case S_SIGSTOP:
//     {
//         // Remove process from scheduler queue, update status, and swap context to run scheduler

//         process->status = STOPPED;
//         process->updated_flag = 1;

//         break;
//     }
//     case S_SIGCONT:
//     {
//         // Add process to scheduler queue, update status, and run scheduler

//         process->status = RUNNING;
//         break;
//     }

//     case S_SIGTERM:
//     {
//         // Add process to parent's zombie queue

//         pid_t *new_zombies = malloc((process->parent->num_zombies + 1) * sizeof(pid_t));
//         for (int i = 0; i < process->parent->num_zombies; ++i)
//         {
//             new_zombies[i] = process->parent->zombies[i];
//         }
//         free(process->parent->zombies);
//         process->parent->num_zombies++;
//         process->parent->zombies = new_zombies;

//         process->status = TERM_SIGNALED;
//         process->updated_flag = 1;

//         break;
//     }
//     }
// }

// currently inactive
// void k_process_cleanup(struct pcb *process)
// {
//     free(process->context->uc_stack.ss_sp);
//     free(process->context);
//     free(process->child_pids);
//     free(process->zombies);
//     free(process);
// }

/**
 * NAME          k_process_cleanup
 * DESCRIPTION   called when a terminated/finished thread’s resources needs to be cleaned up. Such clean-up may include freeing memory, setting the status of the child, etc,
 * FOUND IN      kernel_func.c
 * RETURNS       does not return.
 * ERRORS        throws no errors.
 */
void k_process_cleanup_1(struct pcb *process, char *command)
{
    for (int i = 0; i < process->num_children; i++)
    {
        pcb *child = get_pcb_from_pid(head, process->child_pids[i]);
        if (child)
        {
            k_process_cleanup_1(child, "ORPHAN");
        }
    }

    queues[process->priority + 1] = soft_remove(queues[process->priority + 1], process->pid);

    head = soft_remove(head, process->pid);

    if (process->parent != NULL)
    {
        pid_t *temp = (pid_t *)malloc(sizeof(pid_t) * process->parent->num_children - 1);
        int p2 = 0;
        for (int i = 0; i < process->parent->num_children; i++)
        {
            if (process->parent->child_pids[i] != process->pid)
            {
                temp[p2] = process->parent->child_pids[i];
                p2++;
            }
        }
        process->parent->child_pids = temp;
        process->parent->num_children--;
    }

    // free(process->context->uc_stack.ss_sp);
    // free(process->context);
    // free(process->child_pids);
    // free(process->zombies);

    if (strcmp(command, "NULL") != 0)
        log_command(command, process, 0);

    // free(process);
}

/**
 * NAME          k_process_kill
 * DESCRIPTION   kill the process referenced by process with the signal signal.
 * FOUND IN      kernel_func.c
 * RETURNS       does not return.
 * ERRORS        throws no errors.
 */
void k_process_kill(struct pcb *process, int signal)
{

    wake_up_parent(process);
    if (signal == S_SIGSTOP)
    {
        log_command("STOPPED", process, 0);
        process->status = STOPPED;
        pcb_list_node *queue = queues[process->priority + 1];
        queue = soft_remove(queue, process->pid);
    }

    if (signal == S_SIGTERM)
    {
        log_command("SIGNALED", process, 0);
        process->status = ZOMBIED;
        process->term_status = TERM_SIGNALED;
        // pcb_list_node *queue = queues[process->priority + 1];
        queues[process->priority + 1] = soft_remove(queues[process->priority + 1], process->pid);
        // head = remove_pcb_from_pid(head, process->pid);
    }

    if (signal == S_SIGCONT)
    {
        log_command(CONTINUED, process, 0);
        if (ticks < process->blocked_until)
        {
            log_command(BLOCKED_LOG, process, 0);
            process->status = BLOCKED;
        }
        else
        {
            process->status = RUNNING;
            queues[process->priority + 1] = add_pcb(queues[process->priority + 1], process);
        }
    }
}

void wake_up_parent(pcb *process)
{
    pcb *parent = get_pcb_from_pid(head, process->parent->pid);
    if (parent != NULL && parent->waiting && (parent->waiting_for == process->pid || parent->waiting_for == -1))
    {
        parent->woke_up_by = process->pid;
        parent->status = RUNNING;
        parent->waiting = false;
        queues[parent->priority + 1] = add_pcb(queues[parent->priority + 1], parent);
        log_command(UNBLOCKED, parent, 0);
    }
    if (parent != NULL && parent->waiting_for == -1)
    {
        for (int i = 0; i < parent->num_to_wait_for; i++)
        {
            if (process->pid == parent->to_wait_for[i])
            {
                return;
            }
        }

        pid_t *temp = (pid_t *)malloc(sizeof(pid_t) * (parent->num_to_wait_for + 1));
        int p2 = 0;
        for (int i = 0; i < parent->num_to_wait_for; i++)
        {
            temp[p2] = process->parent->to_wait_for[i];
            p2++;
        }
        temp[parent->num_to_wait_for] = process->pid;
        parent->to_wait_for = temp;
        parent->num_to_wait_for++;
    }
}