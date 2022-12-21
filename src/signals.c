#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "signals.h"
#include "user_func.h"
#include "sched.h"
#include "log.h"

void sigalrm_handler(int signo)
{
    if (signo == SIGALRM)
    {
        if (active_pcb != NULL)
        {
            active_pcb->time_expired = 1;
            swapcontext(active_pcb->context, &scheduler_context);
        }
        else
        {
            setcontext(&scheduler_context);
        }
    }
}

void sigstop_handler(int signo)
{
    if (terminal_control <= 0) 
    {
        printf("\n");
        write(1, PROMPT, strlen(PROMPT));
        return;
    }
    
    if (signo == SIGTSTP)
    {
        p_kill(terminal_control, S_SIGSTOP);
        active_pcb = NULL;
        setcontext(&scheduler_context);
    }
}

void sigint_handler(int signo)
{
    if (terminal_control <= 0) 
    {
        printf("\n");
        write(1, PROMPT, strlen(PROMPT));
        return;
    }
        

    if (signo == SIGINT)
    {
        p_kill(terminal_control, S_SIGTERM);
        active_pcb = NULL;
        setcontext(&scheduler_context);
    }
}