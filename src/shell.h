#ifndef SHELL_H
#define SHELL_H

#define COMMAND_LENGTH 4096
#define RETURN_ASCII_CODE 10

#define SHELL_STOPPED 101
#define SHELL_RUNNING 102
#define SHELL_RESTARTING 103
#define SHELL_BACKGROUND_TO_FOREGROUND 104
#define SHELL_FINISHED 105

#include "stdbool.h"

// Queue is a queue that stores integers that is
// implmented with a singly linked list
// a user can create a queue, add integers to the end,
// remove integers from the front, clear the queue,
// and print all values in it.

void shell(int argc, char *arg[]);

typedef struct Group
{
    int id;
    int *ids;
    char *name;
    int status;
    int size;
    bool changed;

} Group;

typedef struct Node
{
    struct Node *next;
    struct Node *prev;
    Group *group;
} Node;

typedef struct List
{
    Node *head;
    Node *tail;
} List;

void execute(char **command, int outputFile, int inputFile, int readFd[], int writeFd[], bool writing, bool reading, Group *group, int index);

// Normally it is good practice to have comments
// in here to explain the behaviour of each function
void Init(List *s);
void RemoveById(int id, List *s);
void Remove(Node *node, List *s);
void Add(List *s, Group *g);
bool isEmpty(List *s);
Node *Peek(List *s);
void printAll(List *list);
void clear(List *list);

void report(List *list);

void updateStatuses(List *list);

void printJobs(List *list);

#endif

extern int childId;
extern int childStatus;
extern int lastJob;