#include "built_ins.h"
#include <signal.h>
#include "signals.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "kernel.h"
#include <unistd.h>
#include "user_func.h"
#include "filefunc.h"

void idle_process()
{
    while (1)
    {
        sigset_t mask;
        sigsuspend(&mask);
    }
}

void invalid_cmd_process()
{
    printf("not a command\n");
}

void busy_process()
{
    while (1)
    {
    }
}

void sleep_process(char **args)
{
    if (!args[0] || atoi(args[0]) == 0)
    {
        return;
    }
    int num = atoi(args[0]);
    p_sleep(num * 10);
    return;
}

void zombie_child()
{
    // MMMMM Brains...!
    return;
}
void zombify()
{
    p_spawn(zombie_child, NULL, 0, 1, "zombie_child");
    while (1)
        ;
    return;
}

void orphan_child()
{
    // Please sir,
    // I want some more
    while (1)
        ;
}
void orphanify()
{
    p_spawn(orphan_child, NULL, 0, 1, "orphan_child");
    return;
}

void ps_process()
{
    printf("PID\tPPID\tPRI\tSTAT\tCMD\n");
    pcb_list_node *curr = head;
    while (curr)
    {
        int pid = curr->pcb->pid;
        int ppid = 0;
        if (curr->pcb->parent)
        {
            ppid = curr->pcb->parent->pid;
        }
        char status;
        switch (curr->pcb->status)
        {
        case BLOCKED:
            status = 'B';
            break;
        case RUNNING:
            status = 'R';
            break;
        case STOPPED:
            status = 'S';
            break;
        case ZOMBIED:
            status = 'Z';
            break;
        }
        printf("%d\t%d\t%d\t%c\t%s\n", pid, ppid, curr->pcb->priority, status, curr->pcb->name);
        curr = curr->next;
    }
}

void kill_process(char **args)
{
    int count_args = 0;
    int pid = -1;
    while (args[count_args] != NULL)
    {
        count_args++;
    }
    if (count_args == 1 || strcmp(args[0], "-term") == 0)
    {
        if (count_args == 1)
            pid = atoi(args[0]);
        else
            pid = atoi(args[1]);

        printf("%d\n", pid);
        p_kill(pid, S_SIGTERM);
    }
    else if (strcmp(args[0], "-stop") == 0)
    {
        int pid = atoi(args[1]);
        p_kill(pid, S_SIGSTOP);
    }
    else
    {
        int pid = atoi(args[1]);
        p_kill(pid, S_SIGCONT);
    }
}

void echo_wrapper(char **args)
{
    int i = 0;
    int output = active_pcb->fds[1];
    // printf("fd: %d\n", output);
    while (args[i] != NULL)
    {
        // printf("word to write: %s\n", args[i]);
        if (output == 1)
        {
            printf("%s ", args[i]);
        }
        else
        {
            f_write(output, args[i], strlen(args[i]));
            if (args[i + 1] != NULL)
            {
                f_write(output, " ", strlen(" "));
            }
        }
        i++;
    }
}

void ls_wrapper(char **args)
{
    int output = active_pcb->fds[1];
    // printf(" ------ //// %s\n", args[0]);
    if (args[0] == NULL)
    {
        // printf("hello\n");
        f_ls(NULL, output);
    }
    else
    {
        f_ls(args[0], output);
    }
}

void cat_wrapper(char **args)
{
    int i = 0;
    while (args[i] != NULL)
    {
        i++;
    }
    char **files = malloc(sizeof(char *) * (i + 1));
    for (int j = 0; j < i; j++)
    {
        files[j] = args[j];
    }
    files[i] = NULL;
    int output = active_pcb->fds[1];
    // printf("file size: %d\n", i);
    f_cat(files, i, output);
}

void touch_wrapper(char **args)
{
    int i = 0;
    while (args[i] != NULL)
    {
        i++;
    }
    char **files = malloc(sizeof(char *) * (i + 1));
    for (int j = 0; j < i; j++)
    {
        files[j] = args[j];
    }
    files[i] = NULL;
    f_touch(files, i);
}

void mv_wrapper(char **args)
{
    // printf("%s\n", args[0]);
    // printf("%s\n", args[1]);
    f_mv(args[0], args[1]);
}

void rm_wrapper(char **args)
{
    f_rm(args[0]);
}

void cp_wrapper(char **args)
{
    // both are from fs: cp source dest
    // printf("hi this is cp\n");
    f_cp(args[0], args[1]);
}

void chmod_wrapper(char **args)
{
    // check if we are adding or removing a permission
    if (args[0][0] == '+')
    {
        f_chmod(args[1], true, args[0][1]);
    }
    else if (args[0][0] == '-')
    {
        f_chmod(args[1], false, args[0][1]);
    }
    else
    {
        printf("%s\n", "ERROR: Invalid Permissions");
    }
}

char **parse_script(char **args)
{
    // args[0] is the script name
    // printf("%s\n", args[0]);
    char **commands = f_findscript(args[0]);
    return commands;
}