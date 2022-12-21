#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "shell.h"
#include "parser.h"
#include <fcntl.h>
#include "user_func.h"
#include "filefunc.h"
#include "stress.h"
#include "built_ins.h"

// returns pid
int create_child(char **command, char **args, int inputFile, int outputFile)
{
    int pid = -1;
    if (strcmp(command[0], "cat") == 0)
    {
        pid = p_spawn(cat_wrapper, args, inputFile, outputFile, "cat");
    }
    else if (strcmp(command[0], "hang") == 0)
    {
        pid = p_spawn(hang, NULL, 0, 1, *command);
    }
    else if (strcmp(command[0], "nohang") == 0)
    {
        pid = p_spawn(nohang, NULL, 0, 1, *command);
    }
    else if (strcmp(command[0], "recur") == 0)
    {
        pid = p_spawn(recur, NULL, 0, 1, *command);
    }
    else if (strcmp(command[0], "sleep") == 0)
    {
        pid = p_spawn(sleep_process, args, 0, 1, *command);
    }
    else if (strcmp(command[0], "busy") == 0)
    {
        pid = p_spawn(busy_process, NULL, 0, 1, *command);
    }
    else if (strcmp(command[0], "echo") == 0)
    {
        // printf("echo: %s\n", command[0]);
        // printf("args to echo: %s\n", args[0]);
        // if (args[1] != NULL) {
        //     printf("hi im annie\n");
        // }
        pid = p_spawn(echo_wrapper, args, inputFile, outputFile, "echo");
    }
    else if (strcmp(command[0], "ls") == 0)
    {
        pid = p_spawn(ls_wrapper, args, inputFile, outputFile, "ls");
    }
    else if (strcmp(command[0], "touch") == 0)
    {
        pid = p_spawn(touch_wrapper, args, 0, 1, "touch");
    }
    else if (strcmp(command[0], "mv") == 0)
    {
        pid = p_spawn(mv_wrapper, args, 0, 1, "mv");
    }
    else if (strcmp(command[0], "cp") == 0)
    {
        pid = p_spawn(cp_wrapper, args, 0, 1, "cp");
    }
    else if (strcmp(command[0], "rm") == 0)
    {
        pid = p_spawn(rm_wrapper, args, 0, 1, "rm");
    }
    else if (strcmp(command[0], "chmod") == 0)
    {
        pid = p_spawn(chmod_wrapper, args, 0, 1, "chmod");
    }
    else if (strcmp(command[0], "ps") == 0)
    {
        pid = p_spawn(ps_process, NULL, inputFile, outputFile, "ps");
    }
    else if (strcmp(command[0], "kill") == 0)
    {
        pid = p_spawn(kill_process, args, inputFile, outputFile, "kill");
    }
    else if (strcmp(command[0], "zombify") == 0)
    {
        pid = p_spawn(zombify, NULL, 0, 1, "zombie");
    }
    else if (strcmp(command[0], "orphanify") == 0)
    {
        pid = p_spawn(orphanify, NULL, 0, 1, "orphan");
    }
    else
    {
        // printf("hello this is script\n");
        char **commands = parse_script(command);
        // printf("hi\n");

        if (commands == NULL)
        {
            // printf("hi0\n");
            return -1;
        }
        // printf("hi1\n");

        int i = 0;
        while (commands[i] != NULL)
        {
            struct parsed_command *cmd = NULL;
            parse_command(commands[i], &cmd);
            int numArgs = 0;
            int j = 0;
            while (commands[i][j] != '\0')
            {
                if (((commands[i][j] != ' ' && commands[i][j] != '\t' && commands[i][j] != '\n') && commands[i][j + 1] == '\0') ||
                    ((commands[i][j] != ' ' && commands[i][j] != '\t' && commands[i][j] != '\n') && (commands[i][j + 1] == ' ' ||
                                                                                                     commands[i][j + 1] == '\t' || commands[i][j + 1] == '\n')))
                {
                    numArgs += 1;
                }
                j++;
            }

            const char s[2] = " ";
            char *token;
            token = strtok(commands[i], s);
            char **newArgs = malloc(sizeof(char *) * (numArgs));
            // printf("numArgs: %d\n", numArgs);
            int index = 0;
            token = strtok(NULL, s); // skip over the first one.
            while (token != NULL)
            {
                newArgs[index] = token;
                token = strtok(NULL, s);
                index++;
            }
            newArgs[numArgs - 1] = NULL;
            // printf("cmd: %s\n", cmd->commands[0][0]);
            // printf("cmd: %s\n", cmd->commands[0][1]);
            // printf("newArgs: %s\n", newArgs[0]);
            pid = create_child(cmd->commands[0], newArgs, inputFile, outputFile);
            i++;
        }
    }
    return pid;
}

/**
 *  @brief Creates a new process for the given command, sets the file descriptors and modifies the group struct as necessary
 *
 *  @param command the command to execute
 *  @param outputFile The redirection file it is reading from. -1 if there is no redirection input.
 *  @param inputFile The redirection file it is writing to. -1 if there is no redirection output.
 *  @param readFd The descriptors of the file it is reading to if there is a pipeline.
 *  @param readFd The descriptors of the file it is writing to if there is a pipeline.
 *  @param reading Wether or not we are going to read from the pipe file or not (true for all process in a pipe except the first)
 *  @param writing Wether or not we are going to write to the pipe file or not (true for all process in a pipe except the last)
 *  @param group The group struct of the pipeline
 *  @param index The index of the process within the struct
 */
void execute(char **command_, int outputFile, int inputFile, int readFd[], int writeFd[], bool writing, bool reading, Group *group, int index)
{
    // char **args = (char **)malloc(sizeof(char *) * 5);
    int pri = -10;

    int j = 1;
    // char *arg = command[1];
    while (command_[j - 1] != NULL)
    {
        j++;
    }
    char **command;

    if (strcmp(command_[0], "nice") == 0)
    {
        int p2 = 0;
        command = (char **)malloc(sizeof(char *) * (j - 2));
        j -= 2;
        pri = atoi(command_[1]);
        for (; j >= 2; j--)
        {
            // printf("\n\n--- p2: %d, j: %d, string: %s\n\n", p2, j, command_[j]);
            command[p2] = command_[j];
            p2++;
        }
        command[p2] = NULL;
    }
    else
    {
        command = (char **)malloc(sizeof(char *) * j);
        j--;
        command[j] = NULL;
        j--;
        for (; j >= 0; j--)
        {
            command[j] = command_[j];
        }
    }

    // process args ----
    int i = 1;
    // char *arg = command[1];
    while (command[i] != NULL)
    {
        i++;
    }

    char **args = (char **)malloc(sizeof(char *) * i);
    i--;
    args[i] = NULL;
    i--;
    for (; i >= 0; i--)
    {
        args[i] = command[i + 1];
    }

    pid_t pid = -1;
    pid = create_child(command, args, inputFile, outputFile);
    // printf("%d\n", pid);
    if (pid == -1)
    {
        pid = p_spawn(invalid_cmd_process, NULL, 0, 1, *command);
    }

    group->ids[index] = pid;
    group->id = pid;

    if (pri > -10)
    {
        p_nice(pid, pri);
    }

    // printf("Spawned process %s with pid %d\n", *command, pid);

    // pid = fork();
    // if (pid == 0)
    // {
    //     // change stdin if there is redirection
    //     if (inputFile > 0)
    //     {
    //         dup2(inputFile, STDIN_FILENO);
    //         close(inputFile);
    //     }

    //     // change stdout if there is redirection
    //     if (outputFile > 0)
    //     {
    //         dup2(outputFile, STDOUT_FILENO);
    //         close(outputFile);
    //     }

    //     // change stdin if it is reading from pipe and close the other end
    //     if (reading)
    //     {
    //         int ret = dup2(readFd[0], STDIN_FILENO); // read from standard in
    //         close(readFd[1]);
    //         if (ret < 0)
    //             perror("dup2");
    //     }

    //     // change stdout if it is writing to pipe and close the other end
    //     if (writing)
    //     {

    //         int ret = dup2(writeFd[1], STDOUT_FILENO); // read from standard in
    //         close(writeFd[0]);
    //         if (ret < 0)
    //             perror("dup2");
    //     }

    //     int execStatus = execvp(command[0], command);
    //     if (execStatus == -1)
    //     {
    //         perror("Execution error");
    //         exit(EXIT_FAILURE);
    //     }
    //     return 1;
    // }
    // else if (pid > 0)
    // {
    //     // when the id of the group is -1, this means it hasn't been
    //     // changed yet and we are at the first process, we set the group
    //     // id to be the first process id
    //     if (group->id == -1)
    //     {
    //         setpgid(pid, 0);
    //         group->id = pid;
    //     }

    //     // once group id is not -1, the group id has been set with the first process and we want to set
    //     // everything else in the group to have the parent id as the id of the first process
    //     else
    //     {
    //         setpgid(pid, group->id);
    //     }

    //     // add the process id to the ids list of the group
    //     group->ids[index] = pid;

    //     if (reading)
    //     {
    //         // close pipes on the parent
    //         close(readFd[0]);
    //         close(readFd[1]);
    //     }

    //     return 1;
    // }
    // else
    // {
    //     perror("Fork error");
    //     exit(EXIT_FAILURE);
    //     return 1;
    // }
}