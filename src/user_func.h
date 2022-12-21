#ifndef USER_FUNC_H
#define USER_FUNC_H

#include "pcb.h"
#include "pcb_list.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#include <valgrind/valgrind.h>

extern struct pcb_list_node *head;
extern struct pcb *active_pcb;
extern struct pcb_list_node *queues[3];

void p_perror(char* toPrint);

bool check_pid_is_child(struct pcb *parent, int pid);

pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1, char *name);

pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang);

int p_kill(pid_t pid, int sig);

void p_nice(pid_t pid, int priority);

void p_exit();

void p_sleep(unsigned int len);

bool W_WIFEXITED(int status);

bool W_WIFSTOPPED(int status);

bool W_WIFSIGNALED(int status);

#endif
