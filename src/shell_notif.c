#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "shell.h"
#include "log.h"
#include "user_func.h"
#include "kernel_func.h"

/**
 *  @brief  Print updates about all the jobs before prompting the user. Will only print the jobs whose status has changed
 *
 *  @param list the linked list with all the jobs
 *  @return void.
 */
void report(List *list)
{

    Node *current = list->head;
    printf("\n");
    while (current != NULL)
    {
        if (current->group->changed)
        {
            current->group->changed = 0;
            if (current->group->status == SHELL_STOPPED)
            {
                printf("Stopped: %s\n", current->group->name);
            }
            else if (current->group->status == SHELL_RESTARTING)
            {
                printf("Restarting: %s\n", current->group->name);
            }
            else if (current->group->status == SHELL_RUNNING)
            {
                printf("Running: %s\n", current->group->name);
            }
            else if (current->group->status == SHELL_FINISHED)
            {
                printf("Finished: %s\n", current->group->name);
                int prevId = current->group->id;
                current = current->next;
                RemoveById(prevId, list);
                continue;
            }
            else if (current->group->status == SHELL_BACKGROUND_TO_FOREGROUND)
            {
                write(1, current->group->name, strlen(current->group->name));
            }
        }

        current = current->next;
    }
}

/**
 *  @brief  This will update the status of the jobs. It will loop over the struct and kill all
 * zombies and will update their status accordingly so that when will report statuses
 * the next time, we correctly tell the user
 *
 *  @param list the linked list with all the jobs
 *  @return void.
 */
void updateStatuses(List *list)
{
    Node *current = list->head;
    while (current != NULL)
    {
        // first assume, the process is finished and is not stopped
        int dead = 0;
        int stopped = 0;
        for (int i = 0; i < current->group->size; i++)
        {
            int status;
            p_waitpid(current->group->ids[i], &status, true);
            if (W_WIFSTOPPED(status))
            {
                // if at least one process is stopped, the entire process is stopped
                stopped = 1;
                break;
            }
            if (W_WIFEXITED(status) || W_WIFSIGNALED(status))
            {
                // if at least one process is stopped, the entire process is stopped
                dead = 1;
                break;
            }
        }

        // update the status accordingly
        if (dead)
        {
            current->group->changed = 1;
            current->group->status = SHELL_FINISHED;
            pcb *process = get_pcb_from_pid(head, current->group->ids[0]);
            // log_command(WAITED, process, 0);
            printf("cleaning up\n");
            k_process_cleanup_1(process, "NULL");
        }

        if (stopped)
        {
            current->group->changed = 1;
            current->group->status = SHELL_STOPPED;
        }

        current = current->next;
    }
}

/**
 *  @brief  This prints all the jobs in the linked list with their indices and status.
 *  It should be called when the user enters the command jobs
 *
 *  @param list the linked list with all the jobs
 *  @return void.
 */
void printJobs(List *list)
{
    write(1, "\n", strlen("\n"));
    Node *current = list->head;
    int num = 1;
    while (current != NULL)
    {
        char *message = "running";
        if (current->group->status == SHELL_STOPPED)
        {
            message = "stopped";
        }
        if (current->group->status == SHELL_FINISHED)
        {
            message = "finished";
        }
        if (current->group->status == SHELL_RESTARTING)
        {
            message = "restarting";
        }
        printf("[%d] %s (%s)\n", num, current->group->name, message);
        num++;
        current = current->next;
    }
}