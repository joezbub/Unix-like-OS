#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h> /* for setitimer */
#include <signal.h>   /* for signal */
#include "signals.h"
#include "kernel.h"
#include "sched.h"
#include "pcb.h"
#include "pcb_list.h"
#include "user_func.h"
#include "sched.h"
#include "log.h"
#include "built_ins.h"
#include "shell.h"
#include "filefunc.h"

#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <stdio.h>    // dprintf, fputs, perror
#include <ucontext.h> // getcontext, makecontext, setcontext, swapcontext
#include <unistd.h>   // read, usleep, write
#include <valgrind/valgrind.h>

static const int centisecond = 10000; // 10 milliseconds
static const char *LOG_FILE_PATH = "../log/log.txt";

struct pcb_list_node *head = NULL;
int max_pid;
int ticks = 0;
ucontext_t scheduler_context;
struct pcb *active_pcb;
ucontext_t main_context;
FILE *log_file = 0;
ucontext_t idle_context;
pid_t terminal_control = 0;

static void
set_stack(stack_t *stack)
{
    void *sp = malloc(SIGSTKSZ);
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);

    *stack = (stack_t){.ss_sp = sp, .ss_size = SIGSTKSZ};
}

// test function
// static void func()
// {
//     for (int i = 0; i < 3; i++)
//     {
//         // printf("In func: sleeing\n");
//         sleep(1);
//         // printf("In func: %d\n", i);
//     }
//     // printf("Done\n");
// }

static void make_context(ucontext_t *ucp, void (*func)())
{
    getcontext(ucp);
    sigemptyset(&ucp->uc_sigmask);
    set_stack(&ucp->uc_stack);
    ucp->uc_link = &scheduler_context;
    makecontext(ucp, func, 0);
}

static void set_alarm_handler()
{
    struct sigaction act;
    act.sa_handler = sigalrm_handler;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);
}

static void set_timer()
{
    struct itimerval it;
    it.it_interval = (struct timeval){.tv_usec = centisecond * 10};
    it.it_value = it.it_interval;

    setitimer(ITIMER_REAL, &it, NULL);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigstop_handler);
    signal(SIGTTOU, SIG_IGN);

    switch (argc)
    {
    case 1:
        fileFuncConstructor("minfs");
        log_file = fopen(LOG_FILE_PATH, "w");
        break;
    case 2:
        fileFuncConstructor(argv[1]);
        log_file = fopen(LOG_FILE_PATH, "w");
        break;
    case 3:
        fileFuncConstructor(argv[1]);
        char src[50] = "../log/";
        strcat(src, argv[2]);
        log_file = fopen(src, "w");
        break;
    }

    active_pcb = (struct pcb *)malloc(sizeof(struct pcb));
    active_pcb->pid = 0;

    // p_spawn(func, NULL, 0, 1, "test_function_1");
    // int process_id_2 = p_spawn(func, NULL, 0, 1, "test_function_2");
    // p_spawn(func, NULL, 0, 1, "test_function_3");
    int process_id_4 = p_spawn(idle_process, NULL, 0, 1, "idle_process");
    idle_context = *get_pcb_from_pid(head, process_id_4)->context;
    head = soft_remove(head, process_id_4);
    queues[1] = soft_remove(queues[1], process_id_4);

    max_pid = 0;
    int shell_pid = p_spawn(shell, NULL, 0, 1, "shell");
    p_nice(shell_pid, -1);

    // p_nice(process_id_2, -1);
    //  p_sleep(3, process_id_1);

    set_timer();

    set_alarm_handler();

    make_context(&scheduler_context, schedule);
    sigaddset(&scheduler_context.uc_sigmask, SIGALRM);

    swapcontext(&main_context, &scheduler_context);

    if (head != NULL)
    {
        printf("uhoh: check big process queue empty\n");
    }

    if (queues[1] != NULL)
    {
        printf("uhoh: check scheduler queue empty\n");
    }

    // printf("Back to main context\n");

    fclose(log_file);

    free(scheduler_context.uc_stack.ss_sp);
}