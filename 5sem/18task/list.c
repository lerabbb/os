#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mutex.h"

int init(List **head, char *str){
    int err;
    (*head) = (List*) malloc(sizeof(List));

    if((err=init_mutex(&(*head)->mutex))){
        return err;
    }

    (*head)->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy((*head)->buf, str);
    (*head)->next = NULL;
    return 0;
}

int push(char *str, List **head){
    List *temp = (List*) malloc(sizeof(List));
    int err;
    if((err=init_mutex(&(*head)->mutex))){
        return err;
    }

    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);

    lock(&(*head)->mutex);
    temp->next = (*head);
    (*head) = temp;
    unlock(&(*head)->next->mutex);
    return 0;
}

int pop(List **head){
    List *temp =NULL;
    if((*head) == NULL){
        return 1;
    }

    temp = (*head);
    (*head)=(*head)->next;
    destroy(&temp->mutex);
    free(temp->buf);
    free(temp);
    return 0;
}

int printList(List **head){
    lock(&(*head)->mutex);
    printf("head locked to print list\n");
    List *temp = (*head);
    unlock(&(*head)->mutex);
    printf("head unlocked to print list\n");

    printf("List: ");
    while(temp){
        printf("\'%s\' -> ", temp->buf);
        temp=temp->next;
    }
    printf("\n");
    return 0;
}

void swap(List **prev, List **a, List **b) {
    List *temp = NULL;
    printf("\ndata to swap: %s %s\n", (*a)->buf, (*b)->buf);

    if ((*a) == (*prev)) {
        lock(&(*a)->mutex);
        lock(&(*b)->mutex);
        temp = (*b)->next;
        (*b)->next = (*a);
        (*a)->next = temp;
        unlock(&(*a)->mutex);
        unlock(&(*b)->mutex);
        return;
    }

    lock(&(*prev)->mutex);
    printf("prev locked to sort\n");
    lock(&(*a)->mutex);
    printf("a locked to sort\n");
    lock(&(*b)->mutex);
    printf("b unlocked to sort\n");

    temp = (*b)->next;
    (*prev)->next = (*b);
    (*b)->next = (*a);
    (*a)->next = temp;

    unlock(&(*prev)->mutex);
    printf("prev unlocked to sort\n");
    unlock(&(*a)->mutex);
    printf("a unlocked to sort\n");
    unlock(&(*b)->mutex);
    printf("b unlocked to sort\n");
}

void sortList(List **head){
    List *prev=NULL;
    if((*head)->next==NULL){
        return;
    }
    for(List* i = (*head); i != NULL; i = i->next){
        prev = (*head);

        for(List* j = (*head); j != i; j = j->next){
            if(j==NULL || j->next == NULL){
                break;
            }
            if(strcmp(j->buf, j->next->buf) > 0){
                swap(&prev, &j, &j->next);
            }
            prev = j;
        }
    }
}
