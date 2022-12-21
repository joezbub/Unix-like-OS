#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include "pcb.h"

extern FILE *log_file;

void log_command(char *command, pcb *process, int prev_nice);

#define SCHEDULE "SCHEDULE"
#define CREATE "CREATE"
#define SIGNALED "SIGNALED"
#define EXITED "EXITED"
#define ZOMBIE "ZOMBIE"
#define NICE "NICE"
#define ORPHAN "ORPHAN"
#define WAITED "WAITED"
#define BLOCKED_LOG "BLOCKED"
#define UNBLOCKED "UNBLOCKED"
#define STOPPED_LOG "STOPPED"
#define CONTINUED "CONTINUED"

#endif // !LOG_H