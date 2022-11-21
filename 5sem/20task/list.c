#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int init(char *str, List **head){
    (*head) = (List*)malloc(sizeof(List));

    if(pthread_rwlock_init(&(*head)->rwlock, NULL)){
        return 1;
    }

    (*head)->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy((*head)->buf, str);
    (*head)->next=NULL;
    return 0;
}

int push(char *str, List **head){
    List *temp = (List*) malloc(sizeof(List));

    if(pthread_rwlock_init(&(*head)->rwlock, NULL)){
        return 1;
    }

    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);

    pthread_rwlock_wrlock(&(*head)->rwlock);
    temp->next = (*head);
    (*head) = temp;
    pthread_rwlock_unlock(&(*head)->next->rwlock);
    return 0;
}

int pop(List **head){
    List *temp =NULL;
    if((*head) == NULL){
        return 1;
    }

    temp = (*head);
    (*head)=(*head)->next;
    pthread_rwlock_destroy(&temp->rwlock);
    free(temp->buf);
    free(temp);
    return 0;
}

void printList(List **head){
    List *temp = NULL;
    pthread_rwlock_rdlock(&(*head)->rwlock);
    temp = (*head);
    pthread_rwlock_unlock(&(*head)->rwlock);

    printf("List: ");
    while(temp){
        printf("\'%s\' -> ", temp->buf);
        temp=temp->next;
    }
    printf("\n");
}

void swap(List **head, List *prev, List *a, List *b){
    List *temp = NULL;

    if(a == prev){
        pthread_rwlock_wrlock(&a->rwlock);
        pthread_rwlock_wrlock(&b->rwlock);
        temp = b->next;
        b->next = a;
        a->next = temp;
        (*head) = b;
        pthread_rwlock_unlock(&a->rwlock);
        pthread_rwlock_unlock(&b->rwlock);
        return;
    }

    pthread_rwlock_wrlock(&prev->rwlock);
    pthread_rwlock_wrlock(&a->rwlock);
    pthread_rwlock_wrlock(&b->rwlock);
    temp = b->next;
    prev->next = b;
    b->next = a;
    a->next = temp;
    pthread_rwlock_unlock(&prev->rwlock);
    pthread_rwlock_unlock(&a->rwlock);
    pthread_rwlock_unlock(&b->rwlock);
}

void sortList(List **head){
    List *prev, *i, *j;
    prev=NULL;
    j=NULL;
    pthread_rwlock_rdlock(&(*head)->rwlock);
    i=(*head);
    pthread_rwlock_unlock(&(*head)->rwlock);

    while(i){
        pthread_rwlock_rdlock(&(*head)->rwlock);
        prev=(*head);
        j=(*head);
        pthread_rwlock_unlock(&(*head)->rwlock);

        while(j != i && j!=NULL && j->next!=NULL){
            if(strcmp(j->buf, j->next->buf)>0){
                swap(head, prev, j, j->next);
            }
            prev = j;
            j = j->next;
        }

        i=i->next;
    }
}
