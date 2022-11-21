#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void push(char *str, List **head){
    List *temp = (List*) malloc(sizeof(List));
    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);

    temp->next = (*head);
    (*head) = temp;
}

int pop(List **head){
    List *temp =NULL;
    if((*head) == NULL){
        return 1;
    }

    temp = (*head);
    (*head)=(*head)->next;
    free(temp->buf);
    free(temp);
    return 0;
}

void printList(List *head){
    printf("List: ");
    while(head){
        printf("\'%s\' -> ", head->buf);
        head=head->next;
    }
    printf("\n");
}

void swap(List **head, List *prev, List *a, List *b){
    List *temp = NULL;
    if(a == prev){
        temp = b->next;
        b->next = a;
        a->next = temp;
        (*head) = b;
        return;
    }

    temp = b->next;
    prev->next = b;
    b->next = a;
    a->next = temp;
}

int sortList(List **head){
    List *prev=NULL;
    for(List *i = (*head); i; i = i->next){
        prev=(*head);
        for(List *j = (*head); j != i; j = j->next){
            if(!j || !j->next){
                break;
            }
            if(strcmp(j->buf, j->next->buf)>0){
                swap(head, prev, j, j->next);
            }
            prev = j;
        }
    }
}
