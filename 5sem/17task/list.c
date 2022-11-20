#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void push(char *str, List **head){
    List *temp;
    temp = (List*) malloc(sizeof(List));
    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);
    temp->prev = NULL;
    temp->next = (*head);
    if((*head)){
        (*head)->prev = temp;
    }
    (*head) = temp;
}

int pop(List **head){
    List *temp =NULL;
    if((*head) == NULL){
        return 1;
    }

    temp = (*head);
    (*head)=(*head)->next;
    //(*head)->prev = NULL;
    free(temp);
    return 0;
}

void printList(List *head){
    List *temp = head;
    printf("List: ");
    while(temp){
        printf("\'%s\' -> ", temp->buf);
        temp=temp->next;
    }
    printf("\n");
}

int swap(List **head, List **a, List **b){
    List *temp = NULL;
    if(!(*head) || !(*a) || !(*b)){
        return 1;
    }
    if((*a) == (*head)) {
        *head = *b;
    }
    else if((*b) == (*head)){
        *head = *a;
    }

    temp = (*a)->next;
    (*a)->next = (*b)->next;
    (*b)->next = temp;
    if((*b)->next){
        (*b)->next->prev = (*b);
    }
    if((*a)->next){
        (*a)->next->prev = (*a);
    }

    temp = (*a)->prev;
    (*a)->prev = (*b)->prev;
    (*b)->prev = temp;
    if((*b)->prev){
        (*b)->prev->next = (*b);
    }
    if((*a)->prev){
        (*a)->prev->next = (*a);
    }

    return 0;
}

int sortList(List **head){
    for(List* i = (*head); i!=NULL; i = i->next){
        for(List* j = i->next; j; j = j->next){
            if(strcmp(i->buf, j->buf) > 0){
                if(swap(head, &i, &j))
                    return 1;
            }
        }
    }
    return 0;
}
