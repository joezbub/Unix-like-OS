#include "pcb.h"
#include "log.h"
#include "pcb_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/types.h>
#include <ucontext.h>

struct pcb_list_node *add_pcb(struct pcb_list_node *head, struct pcb *pcb)
{
    struct pcb_list_node *new_node = (struct pcb_list_node *)malloc(sizeof(struct pcb_list_node));
    new_node->pcb = pcb;
    new_node->next = NULL;
    if (head == NULL)
    {
        return new_node;
    }
    else
    {
        struct pcb_list_node *tmp = head;
        while (tmp->next)
        {
            tmp = tmp->next;
        }
        tmp->next = new_node;
        return head;
    }
}

// Return pcb with pid if exists
struct pcb *get_pcb_from_pid(struct pcb_list_node *head, int pid)
{
    while (head)
    {
        if (head->pcb->pid == pid)
        {
            return head->pcb;
        }
        head = head->next;
    }
    return NULL;
}

// Remove pcb from list if exists, return new head
struct pcb_list_node *remove_pcb_from_pid(struct pcb_list_node *head, int pid)
{

    struct pcb_list_node *prev = NULL, *curr = head;
    while (curr)
    {
        if (curr->pcb->pid == pid)
        {
            pcb_list_node *next = curr->next;
            // free(curr);
            if (!prev)
            {
                return next;
            }
            else
            {
                prev->next = next;
                return head;
            }
        }
        prev = curr;
        curr = curr->next;
    }
    return head;
}

// Remove pcb from list if exists, return new head
pcb_list_node *soft_remove(pcb_list_node *head, int pid)
{
    struct pcb_list_node *prev = NULL, *curr = head;
    while (curr)
    {
        if (curr->pcb->pid == pid)
        {
            pcb_list_node *next = curr->next;
            if (!prev)
            {
                return next;
            }
            else
            {
                prev->next = next;
                return head;
            }
        }
        prev = curr;
        curr = curr->next;
    }
    return head;
}