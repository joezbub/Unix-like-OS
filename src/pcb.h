#ifndef PCB_H
#define PCB_H

#include <stdbool.h>
#include <sys/types.h>
#include <ucontext.h>

#define TERM_NORMAL 10
#define TERM_SIGNALED 20
#define RUNNING 3
#define STOPPED 4
#define BLOCKED 5
#define ZOMBIED 6

/**
 * PCB struct
 */
typedef struct pcb
{
    ucontext_t *context;
    struct pcb *parent;
    pid_t pid;
    int num_children;
    pid_t *child_pids;
    int num_zombies;
    pid_t *zombies;
    int fds[2]; // fd set to -1 as default
    int priority;
    int status;
    int term_status;
    bool waiting;
    int num_to_wait_for;
    pid_t *to_wait_for;
    pid_t woke_up_by;
    pid_t waiting_for;
    bool updated_flag; // flag set to 1 if pcb status was updated
    bool time_expired;
    char *name;
    int blocked_until;
    bool sleeping;
    // add more later
} pcb;

#endif
