#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "shell.h"
#include "kernel.h"
#include "user_func.h"
#include "kernel_func.h"
#include "parser.h"
#include "signals.h"
#include "log.h"
#include "filefunc.h"

void parent_sigtstp_handler(int signo)
{
    if (signo == SIGTSTP)
    {
        printf("\n");
        write(1, PROMPT, strlen(PROMPT));
    }
}

void parent_sigint_handler(int signo)
{
    if (signo == SIGINT)
    {
        printf("\n");
        write(1, PROMPT, strlen(PROMPT));
    }
}

int lastJob = -1;
bool stoppedJobs = false;
int begin = 1;

/**
 *  @brief This function takes a group, runs it in the foreground, waits for it to complete
 * and then returns terminal control to the parent. This is called if a job is run
 * on the foreground or if fg is called.
 *
 * This called if the user runs a job normally in the foreground or when the fg command is called
 *
 *  @param group the group we want to move to the foreground
 *  @return void.
 */
void runOnForeground(Group *group)
{
    // give terminal control to the group
    terminal_control = group->id;
    // tcsetpgrp(STDIN_FILENO, group->id);
    // wait for all the jobs in the group
    int stopped = 0;
    for (int j = 0; j < group->size; j++)
    {
        int status = -1;
        p_waitpid(group->ids[j], &status, false);
        if (W_WIFSTOPPED(status))
        {
            stopped = 1;
            break;
        }
        else
        {
            pcb *process = get_pcb_from_pid(head, group->ids[j]);
            log_command(WAITED, process, 0);
            k_process_cleanup_1(process, "NULL");
        }
    }

    group->changed = true;
    if (stopped)
    {
        group->status = SHELL_STOPPED;
        lastJob = group->id;
    }
    else
    {
        group->status = SHELL_FINISHED;
    }

    // give terminal control back to parent
    terminal_control = 0;

    // tcsetpgrp(STDIN_FILENO, getpid());
}

void shell(int argc, char *arg[])
{
    if (argc > 1)
    {
        char *message = "too many arguments\n";
        write(STDOUT_FILENO, message, strlen(message));
        exit(EXIT_FAILURE);
    }

    // signal(SIGINT, parent_sigint_handler);
    // signal(SIGTSTP, parent_sigtstp_handler);

    List jobs;
    Init(&jobs);

    if (isatty(STDIN_FILENO) == 0)
    {
        char command[COMMAND_LENGTH];
        read(STDIN_FILENO, command, COMMAND_LENGTH);
        printf("%s\n", command);
    }

    // main loop of the program, read the user input and execute it
    while (1)
    {
        if (!begin)
        {
            // report the jobs before prompting the user
            report(&jobs);
        }
        begin = 0;

        write(1, PROMPT, strlen(PROMPT));
        char command[COMMAND_LENGTH];
        int bytesRead = read(STDIN_FILENO, command, COMMAND_LENGTH);
        if (bytesRead == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (bytesRead == 0)
        {
            clear(&jobs);
            p_exit();
            // exit(EXIT_SUCCESS);
        }
        // make sure there is a null terminator at the end of the command
        command[bytesRead] = '\0';

        struct parsed_command *cmd = NULL;

        // update the statuses and kill the zombies

        updateStatuses(&jobs);

        // if the user typed an end of file, print a new line
        // and do not process the command
        if (strlen(command) == 1)
        {
            char *message = "\n";
            write(STDOUT_FILENO, message, strlen(message));
            free(cmd);
            continue;
        }

        int res = parse_command(command, &cmd);
        if (res != 0)
        {
            perror("invalid");
            free(cmd);
            continue;
        }

        // if the command is fg bring the latest job and then call runOnForeground on it
        if (strcmp(cmd->commands[0][0], "fg") == 0)
        {
            // num will be -1 if nothing ahs been inputted or the argument if there one
            int num = -1;
            if (cmd->commands[0][1] != NULL)
                num = atoi(cmd->commands[0][1]);

            // starting counting the jobs until the count is num (the id)
            int count = 1;
            Group *targetGroup;
            Node *current = jobs.head;

            // if we have a specific id, loop until you find it and set target group to it
            if (num != -1)
            {
                while (current != NULL)
                {
                    if (count == num)
                    {
                        targetGroup = current->group;
                    }

                    count++;
                    current = current->next;
                }
            }
            // if not, then loop updating each time you see a job
            // at the end you should be left with targetGroup be equal the last group
            // we start we stopped = false
            // if we see a stopped job we set it to true
            // once it is true, we will be only considering stopped jobs and hence
            // only find the latest stopped job
            else
            {
                bool stopped = false;
                while (current != NULL)
                {
                    if (current->group->status == SHELL_STOPPED)
                    {
                        targetGroup = current->group;
                        stopped = true;
                    }

                    if (current->group->status == SHELL_RUNNING && !stopped)
                    {
                        targetGroup = current->group;
                    }

                    count++;
                    current = current->next;
                }
            }

            // If "fg" called without any jobs
            if (count == 1)
            {
                free(cmd);
                continue;
            }

            // continue the stopped job, write it is name, and run it on the foreground
            for (int i = 0; i < targetGroup->size; i++)
            {
                p_kill(targetGroup->ids[i], S_SIGCONT);
            }
            if (targetGroup->status == SHELL_STOPPED)
            {
                printf("Restarting: %s", targetGroup->name);
            }
            else
            {
                printf("running %s", targetGroup->name);
            }
            runOnForeground(targetGroup);

            free(cmd);
            continue;
        }

        // this is identical to fg, read that first, the only difference is that
        // after continue, we don't move it to the foreground
        if (strcmp(cmd->commands[0][0], "bg") == 0)
        {
            int num = -1;
            if (cmd->commands[0][1] != NULL)
                num = atoi(cmd->commands[0][1]);

            int count = 1;
            Group *targetGroup;
            Node *current = jobs.head;

            if (num != -1)
            {
                while (current != NULL)
                {
                    if (count == num)
                    {
                        targetGroup = current->group;
                    }

                    count++;
                    current = current->next;
                }
            }
            else
            {
                bool stopped = false;
                while (current != NULL)
                {
                    if (current->group->status == SHELL_STOPPED)
                    {
                        targetGroup = current->group;
                        stopped = true;
                    }

                    if (current->group->status == SHELL_RUNNING && !stopped)
                    {
                        targetGroup = current->group;
                    }

                    count++;
                    current = current->next;
                }
            }

            // If "bg" called without any jobs
            if (count == 1)
            {
                free(cmd);
                continue;
            }
            for (int i = 0; i < targetGroup->size; i++)
            {
                // kill(targetGroup->ids[i], SIGCONT);
                p_kill(targetGroup->ids[i], S_SIGCONT);
            }

            targetGroup->status = SHELL_RUNNING;
            printf("Running: %s\n", targetGroup->name);
            free(cmd);
            continue;
        }

        // if the command is jobs, print all jobs
        if (strcmp(cmd->commands[0][0], "jobs") == 0)
        {
            free(cmd);
            printJobs(&jobs);
            continue;
        }

        if (strcmp(cmd->commands[0][0], "nice_pid") == 0)
        {
            pid_t pid = atoi(cmd->commands[0][2]);
            int priority = atoi(cmd->commands[0][1]);
            if (priority >= -1 && priority <= 1)
                p_nice(pid, priority);
            free(cmd);
            continue;
        }

        if (strcmp(cmd->commands[0][0], "man") == 0)
        {
            printf("cat\t\tThe usual cat from bash, etc.\n");
            printf("sleep\t\tsleep for n seconds.\n");
            printf("busy\t\tbusy wait indefinitely.\n");
            printf("echo\t\tsimilar to echo(1) in the VM.\n");
            printf("ls\t\tlist all files  in the working directory (similar to ls -il in bash), same formatting as ls in the standalone PennFAT\n");
            printf("touch\t\tcreate an empty file if it does not exist, or update its timestamp otherwise.\n");
            printf("mv\t\trename source to destination.\n");
            printf("cp\t\tcopy source to destination.\n");
            printf("rm\t\tremove files.\n");
            printf("chmod\t\tsimilar to chmod(1) in the VM.\n");
            printf("ps\t\tlist all processes on PennOS. Display grid, ppid, and priority.\n");
            printf("kill\t\tsend the specified signal to the specified processes, where signal is either term (the default), stop, or cont, corresponding to S_SIGTERM, S_SIGSTOP, and S_SIGCONT, respectively. Similar to /bin/kill in the VM.\n");
            printf("zombify\t\tspawn a zombie child.\n");
            printf("orphanify\t\tspawn an orphan child.\n");
            printf("nice priority\t\tset the priority of the command to priority and execute the command.\n");
            printf("nice_pid priority\t\tadjust the nice level of process pid to priority.\n");
            printf("man\t\tlist all available commands.\n");
            printf("bg\t\tcontinue the last stopped job, or the job specified by the id.\n");
            printf("fg\t\tbring the last stopped or backgrounded job to the foreground, or the job specified by the id.\n");
            printf("jobs\t\tlist all jobs.\n");
            printf("logout\t\texit the shell and shutdown PennOS.\n");
            free(cmd);
            continue;
        }

        if (strcmp(cmd->commands[0][0], "logout") == 0)
        {
            free(cmd);           
            p_exit();
            exit(EXIT_SUCCESS);
            continue;
        }

        // if (strcmp(cmd->commands[0][0], "ps") == 0)
        // {
        //     printf("PS REACHED\n");
        //     // num will be -1 if nothing ahs been inputted or the argument if there one
        //     // starting counting the jobs until the count is num (the id)
        //     Node *current = jobs.head;

        //     while (current != NULL)
        //     {
        //         if (current->group->status == SHELL_STOPPED)
        //         {
        //             printf("STOPPED\n");
        //             printf("ID: %i\n", *current->group->ids);
        //             kill(current->group->id, SIGKILL);
        //             kill(current->group->id + 1, SIGKILL);
        //         }
        //         current = current->next;
        //     }
        // }

        // pipeline file descriptors
        int readFd[2];
        int writeFd[2];

        // intialize the group for the command
        Group *group = (Group *)malloc(sizeof(Group));
        // malloc enough space for the ids of the group
        group->ids = (int *)malloc(sizeof(int) * cmd->num_commands);
        // initalize values that will be changed later
        group->id = -1;
        group->status = 0;
        group->size = cmd->num_commands;
        // copy name
        int len = 0;
        for (int k = 0; k < strlen(command); k++)
        {
            if (command[k] != '&')
            {
                len++;
            }
        }
        group->name = (char *)malloc(sizeof(char) * len + 1);
        int h = 0;
        for (int k = 0; k < strlen(command); k++)
        {
            if (command[k] != '&')
            {
                group->name[h] = command[k];
                h++;
            }
        }
        group->name[h] = '\0';

        // finish copy name
        group->changed = 1;
        // change the status to running and mark that the status has changed
        group->status = SHELL_RUNNING;
        // add the job to the linked list
        Add(&jobs, group);
        for (int i = 0; i < cmd->num_commands; i++)
        {
            // reading and writing tells us if the process is reading or writing
            // to or from a pipe

            bool writing = false;
            bool reading = false;

            // the read pipe for this process will be the write pipe from the
            // last process
            readFd[0] = writeFd[0];
            readFd[1] = writeFd[1];

            if (i != 0)
            {
                // all proceses except first will read
                reading = true;
            }

            if (i < cmd->num_commands - 1)
            {
                // all process except last will write
                // we open new pipes at the writing process

                int pipeStatus = pipe(writeFd);

                if (pipeStatus < 0)
                {
                    perror("pipe");
                }
                writing = true;
            }

            // we open the outpfile for redirection if the process is the last
            int outputFile = 1;
            if (i == cmd->num_commands - 1 && cmd->stdout_file != NULL)
            {
                if (cmd->is_file_append)
                {
                    outputFile = f_open(cmd->stdout_file, false); // false: not overwrite
                }
                else
                {
                    // printf("hi this is overwrite\n");
                    outputFile = f_open(cmd->stdout_file, true);
                }
            }

            int inputFile = 0;
            // we open the input file for redirection if the process is the first
            if (i == 0 && cmd->stdin_file != NULL)
            {
                inputFile = f_open(cmd->stdin_file, false);
            }

            // exectue the process
            execute(cmd->commands[i], outputFile, inputFile, readFd, writeFd, writing, reading, group, i);
        }

        if (!cmd->is_background)
        {
            // if the process is onn the foreground call runOnForeground on it
            runOnForeground(group);
        }

        // update the latest job
        lastJob = group->id;
        free(cmd);
        // printAll(&jobs);
    }
}