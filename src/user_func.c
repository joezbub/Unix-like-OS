
#include "kernel_func.h"
#include "pcb.h"
#include "log.h"
#include "pcb_list.h"
#include "signals.h"
#include "user_func.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#include <valgrind/valgrind.h>
#include <string.h>
#include "errno.h"

void p_perror(char *toPrint)
{
    if (errno == E2BIG)
    {
        printf("%s : %s\n", toPrint, "Argument list too long");
    }
    else if (errno == EFBIG)
    {
        printf("%s : %s\n", toPrint, "File too large");
    }
    else if (errno == ENOENT)
    {
        printf("%s : %s\n", toPrint, "No such file or directory");
    }
    else if (errno == ENAMETOOLONG)
    {
        printf("%s : %s\n", toPrint, "Filename too long");
    }
    else if (errno == EPROCESSDOESNOTEXIST)
    {
        printf("%s : %s\n", toPrint, "Process does not exist");
    }
    else if (errno == EEOF)
    {
        printf("%s : %s\n", toPrint, "Reached end of file");
    }
    else if (errno == EFTF)
    {
        printf("%s : %s\n", toPrint, "File table full");
    }
    else if (errno == EUDWV)
    {
        printf("%s : %s\n", toPrint, "Undefined WHENCE Value");
    }
    else if (errno == EINVPERM)
    {
        printf("%s : %s\n", toPrint, "Invalid date or permissions");
    }
}

bool check_pid_is_child(struct pcb *parent, int pid)
{
    for (int i = 0; i < parent->num_children; ++i)
    {
        if (parent->child_pids[i] == pid)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * NAME          p_spawn
 * DESCRIPTION   forks a new thread that retains most of the attributes of the parent thread (see k_process_create). Once the thread is spawned, it executes the function referenced by func with its argument array argv. fd0 is the file descriptor for the input file, and fd1 is the file descriptor for the output file.
 * FOUND IN      user_func.c
 * RETURNS       child pid on success
 * ERRORS        failed to fork and allocate necessary kernal structures because memory is tight.
 */
pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1, char *name)
{
    int arg_c = 0;
    while (argv && argv[arg_c] != NULL)
    {
        arg_c++;
    }
    struct pcb *pcb = k_process_create(active_pcb);

    pcb->fds[0] = fd0;
    pcb->fds[1] = fd1;

    int len = 0;
    while (name[len] != '\0')
    {
        len++;
    }

    pcb->name = malloc(sizeof(char) * (len + 1));
    strcpy(pcb->name, name);

    log_command(CREATE, pcb, 0);

    if (arg_c < 1)
    {
        arg_c = 1;
    }

    makecontext(pcb->context, func, arg_c, argv);

    queues[1] = add_pcb(queues[1], pcb);
    head = add_pcb(head, pcb);

    return pcb->pid;
}

// currently inactive
// pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang)
// {
//     int caller = 0; // get_pid_from_scheduler();
//     struct pcb *caller_pcb = get_pcb_from_pid(head, caller);
//     if (!check_pid_is_child(caller_pcb, pid))
//     {
//         // pid is not a child of the calling thread
//         return -1;
//     }

//     struct pcb *child_pcb = get_pcb_from_pid(head, pid);

//     if (nohang)
//     {
//         if (child_pcb->updated_flag)
//         {
//             switch (child_pcb->status)
//             {
//             case TERM_NORMAL:
//                 *wstatus = TERM_NORMAL;
//                 break;
//             case TERM_SIGNALED:
//                 *wstatus = TERM_SIGNALED;
//                 break;
//             case RUNNING:
//                 *wstatus = RUNNING;
//                 break;
//             case STOPPED:
//                 *wstatus = STOPPED;
//                 break;
//             case BLOCKED:
//                 *wstatus = BLOCKED;
//                 break;
//             }
//             child_pcb->updated_flag = 0;
//             return pid;
//         }
//         else
//         {
//             return -1;
//         }
//     }
//     else
//     {
//         // Remove caller from scheduler queue (to block)
//         // How to check child if blocked?
//         return -1;
//     }
// }

pid_t p_waitpid_1(pid_t pid, int *wstatus, bool nohang)
{
    bool status_changed = false; // Child status changed
    bool waitable = false;       // Waitable child exists
    if (pid == -1)
    {
        int changed_id = -1;
        if (nohang == true)
        {
            for (int i = 0; i < active_pcb->num_children; i++)
            {
                pcb *child_pcb = get_pcb_from_pid(head, active_pcb->child_pids[i]);
                if (child_pcb->updated_flag)
                {
                    changed_id = child_pcb->pid;
                    status_changed = true;
                    log_command(WAITED, child_pcb, 0);
                    k_process_cleanup_1(child_pcb, WAITED);
                    break;
                }
                if (child_pcb->status != ZOMBIED)
                {
                    waitable = true;
                }
            }
            if (status_changed)
            {
                return changed_id;
            }
            if (waitable)
            {
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            for (int i = 0; i < active_pcb->num_children; i++)
            {
                pcb *child_pcb = get_pcb_from_pid(head, active_pcb->child_pids[i]);
                if (child_pcb->status != ZOMBIED)
                {
                    waitable = true;
                }
            }

            if (waitable)
            {
                if (active_pcb->num_to_wait_for == 0)
                {
                    active_pcb->status = BLOCKED;
                    active_pcb->waiting = true;
                    active_pcb->waiting_for = pid;
                    active_pcb->sleeping = false;
                    queues[active_pcb->priority + 1] = soft_remove(queues[active_pcb->priority + 1], active_pcb->pid);
                    ucontext_t *temp = active_pcb->context;
                    active_pcb = NULL;
                    swapcontext(temp, &scheduler_context);
                }
                pcb *child_pcb = get_pcb_from_pid(head, active_pcb->to_wait_for[0]);
                changed_id = child_pcb->pid;
                pid_t *new_wait_queue = (pid_t *)malloc(sizeof(pid_t) * (active_pcb->num_to_wait_for - 1));
                for (int i = 1; i < active_pcb->num_to_wait_for; i++)
                {
                    new_wait_queue[i - 1] = active_pcb->to_wait_for[i];
                }
                active_pcb->to_wait_for = new_wait_queue;
                active_pcb->num_to_wait_for--;
                k_process_cleanup_1(child_pcb, WAITED);
                return changed_id;
            }
            else
            {
                return -1;
            }
        }
    }
    return 0;
}

/**
 * NAME          p_waitpid
 * DESCRIPTION   sets the calling thread as blocked (if nohang is false) until a child of the calling thread changes state. It is similar to Linux waitpid(2). If nohang is true, p_waitpid does not block but returns immediately.
 * FOUND IN      user_func.c
 * RETURNS       child pid which has changed state on success, or nohang is true and there is no block.
 * ERRORS        child has not changed state on success because child does not have
 */
pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang)
{
    if (pid == -1)
    {
        return p_waitpid_1(pid, wstatus, nohang);
    }
    pcb *child_pcb = get_pcb_from_pid(head, pid);

    if (!child_pcb)
    {
        return -1;
    }

    if (nohang == false)
    {
        log_command(BLOCKED_LOG, active_pcb, 0);
        active_pcb->status = BLOCKED;
        active_pcb->waiting = true;
        active_pcb->waiting_for = pid;
        active_pcb->sleeping = false;
        queues[active_pcb->priority + 1] = soft_remove(queues[active_pcb->priority + 1], active_pcb->pid);
        ucontext_t *temp = active_pcb->context;
        active_pcb = NULL;
        swapcontext(temp, &scheduler_context);
    }

    int child_status = child_pcb->status;

    if (wstatus == NULL)
    {
        int x;
        wstatus = &x;
    }

    if (child_status == ZOMBIED)
    {
        *wstatus = child_pcb->term_status;
    }
    else
    {
        *wstatus = child_status;
    }

    if (child_pcb->status == RUNNING)
    {
        return 0;
    }

    return pid;
}

bool W_WIFEXITED(int status)
{
    return (status == TERM_NORMAL);
}

bool W_WIFSTOPPED(int status)
{
    return (status == STOPPED);
}

bool W_WIFSIGNALED(int status)
{
    return (status == TERM_SIGNALED);
}

/**
 * NAME          p_kill
 * DESCRIPTION   kills a process with specified pid and signal.
 * FOUND IN      user_func.c
 * RETURNS       0 upon success and -1 upon error
 * ERRORS        pid is invalid
 */
int p_kill(pid_t pid, int sig)
{

    struct pcb *pcb = get_pcb_from_pid(head, pid);

    if (!pcb)
    {
        errno = EPROCESSDOESNOTEXIST;
        perror("p_kill()");
        return -1;
    }
    else
    {
        k_process_kill(pcb, sig);
        return 0;
    }

    // printf("id: %d\n", pid);
    // printf("name: %s\n", pcb->name);
}

// currently inactive
// void rec_exit(int pid)
// {
//     struct pcb *pcb = get_pcb_from_pid(head, pid);
//     head = remove_pcb_from_pid(head, pid);
//     for (int i = 0; i < pcb->num_children; ++i)
//     {
//         rec_exit(pcb->child_pids[i]);
//     }
//     k_process_cleanup(pcb);
// }

/**
 * NAME          p_exit
 * DESCRIPTION   exits the current thread unconditionally.
 * FOUND IN      user_func.c
 * RETURNS       successfully exits the terminal.
 * ERRORS        active process does not exist.
 */
void p_exit()
{
    // int pid = active_pcb->pid;
    // rec_exit(pid);
    wake_up_parent(active_pcb);
    k_process_cleanup_1(active_pcb, "EXITED");
    if (terminal_control == 0)
        exit(EXIT_SUCCESS);
}

/**
 * NAME          p_nice
 * DESCRIPTION   sets the priority of the thread pid to priority.
 * FOUND IN      user_func.c
 * RETURNS       returns upon successful completion.
 * ERRORS        fail to retrieve the process pcb.
 */
void p_nice(pid_t pid, int priority)
{
    // modify in the full queue
    pcb *process_pcb = get_pcb_from_pid(head, pid);
    if (!process_pcb)
    {
        return;
    }
    int previous_priority = process_pcb->priority;
    process_pcb->priority = priority;

    log_command(NICE, process_pcb, previous_priority);

    if (previous_priority == process_pcb->priority)
        return;

    // modify from scheduling queues
    queues[previous_priority + 1] = soft_remove(queues[previous_priority + 1], process_pcb->pid);
    queues[priority + 1] = add_pcb(queues[priority + 1], process_pcb);
}

/**
 * NAME          p_sleep
 * DESCRIPTION   sets the calling process to blocked until ticks of the system clock elapse, and then sets the thread to running. p_sleep does not return until the thread resumes running; however, it can be interrupted by a S_SIGTERM signal. Like sleep(3) in Linux, the clock keeps ticking even when p_sleep is interrupted.
 * FOUND IN      user_func.c
 * RETURNS       it does not return.
 * ERRORS        throws no errors.
 */
void p_sleep(unsigned int len)
{
    // modify in the full queue
    //  printf("----1\n");
    active_pcb->status = BLOCKED;
    active_pcb->sleeping = true;
    active_pcb->blocked_until = ticks + len;
    queues[active_pcb->priority + 1] = soft_remove(queues[active_pcb->priority + 1], active_pcb->pid);
    log_command(BLOCKED_LOG, active_pcb, 0);
    ucontext_t *temp = active_pcb->context;
    active_pcb = NULL;
    swapcontext(temp, &scheduler_context);
}