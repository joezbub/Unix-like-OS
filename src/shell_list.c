/*
 * queue.c
 */
#include <stdio.h>  // for printf
#include <stdlib.h> // for malloc, free, and EXIT_FAILURE
#include "shell.h"

void Init(List *s)
{
    s->head = NULL;
    s->tail = NULL;
}

void Add(List *s, Group *g)
{
    Node *node = malloc(sizeof(Node));

    if (node == NULL)
    {
        printf("Couldn't allocate space for a new List Node\n");
        free(node);
        exit(EXIT_FAILURE);
    }

    node->group = g;
    node->next = NULL;
    node->prev = NULL;

    if (s->tail == NULL)
    {
        s->head = node;
        s->tail = node;
    }
    else
    {

        node->prev = s->tail;
        s->tail->next = node;
        s->tail = node;
    }

    return;
}

Node *Peek(List *s)
{
    return s->tail;
}

void RemoveById(int id, List *list)
{
    // check if the id exists or not
    Node *current = list->head;
    Node *toRemove = NULL;
    while (current != NULL)
    {
        if (current->group->id == id)
        {
            toRemove = current;
        }
        current = current->next;
    }

    // if not, don't do anything
    if (toRemove == NULL)
    {
        return;
    }

    // There are four cases, the group is first, last, neither or both. Handle each case seperately
    if (toRemove == list->head && toRemove == list->tail)
    {
        list->head = NULL;
        list->tail = NULL;
    }
    else if (toRemove == list->head)
    {
        list->head = toRemove->next;
        toRemove->next->prev = NULL;
    }
    else if (toRemove == list->tail)
    {
        list->tail = toRemove->prev;
        toRemove->prev->next = NULL;
    }
    else
    {
        toRemove->prev->next = toRemove->next;
        toRemove->next->prev = toRemove->prev;
    }

    // free all the resources of the group in reverse order
    free(toRemove->group->ids);
    free(toRemove->group->name);
    free(toRemove->group);
    free(toRemove);
}

bool isEmpty(List *s)
{
    if (!s->head)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// useful for debuggin
void printAll(List *list)
{
    Node *n = list->head;
    while (n != NULL)
    {
        printf("------ group id = %d, name = %s\n", n->group->id, n->group->name);
        for (int i = 0; i < n->group->size; i++)
        {
            printf("#%d: %d\n", i, n->group->ids[i]);
        }
        n = n->next;
    }
}

// clear the list useful when something unexpected happens and we need to free our resources before exiting
void clear(List *list)
{
    Node *curr = list->head;
    while (curr != NULL)
    {
        Node *toRemove = curr;
        curr = curr->next;
        RemoveById(toRemove->group->id, list);
    }
}
