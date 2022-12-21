#include "pcb.h"
#include "kernel.h"
#include "sched.h"
#include "log.h"
#include "stdio.h"
#include "string.h"

void log_command(char *command, pcb *process, int prev_nice)
{
    if (strcmp(command, "NICE") == 0)
    {
        fprintf(log_file, "[%d]\t%s\t%d\t%d\t%d\t%s\n", ticks, command, process->pid, prev_nice, process->priority, process->name);
    }
    else if (strcmp(command, "SCHEDULE") == 0)
    {
        fprintf(log_file, "[%d]\t%s\t%d\t%d\t%s\n", ticks, command, process->pid, process->priority, process->name);
    }
    else
    {
        fprintf(log_file, "[%d]\t%s\t%d\t%d\t%s\n", ticks, command, process->pid, process->priority, process->name);
    }

    fflush(log_file);

    // fprintf("[%d]\t%s\t")
}