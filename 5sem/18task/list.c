#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int init(char *str, List **head){
    (*head) = (List*)malloc(sizeof(List));

    if(pthread_mutex_init(&(*head)->mutex, NULL)){
        return 1;
    }

    (*head)->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy((*head)->buf, str);
    (*head)->next=NULL;
    return 0;
}

int push(char *str, List **head){
    List *temp = (List*) malloc(sizeof(List));

    if(pthread_mutex_init(&temp->mutex, NULL)){
        return 1;
    }

    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);

    pthread_mutex_lock(&(*head)->mutex);
    temp->next = (*head);
    (*head) = temp;
    pthread_mutex_unlock(&(*head)->next->mutex);
    return 0;
}

int pop(List **head){
    List *temp =NULL;
    if((*head) == NULL){
        return 1;
    }

    temp = (*head);
    (*head)=(*head)->next;
    pthread_mutex_destroy(&temp->mutex);
    free(temp->buf);
    free(temp);
    return 0;
}

void printList(List **head){
    List *temp = NULL;
    pthread_mutex_lock(&(*head)->mutex);
    temp = (*head);
    pthread_mutex_unlock(&(*head)->mutex);

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
        pthread_mutex_lock(&a->mutex);
        pthread_mutex_lock(&b->mutex);
        temp = b->next;
        b->next = a;
        a->next = temp;
        pthread_mutex_lock(&(*head)->mutex);
        (*head) = b;
        pthread_mutex_unlock(&a->mutex);
        pthread_mutex_unlock(&b->mutex);
        pthread_mutex_unlock(&(*head)->mutex);
        return;
    }

    pthread_mutex_lock(&prev->mutex);
    pthread_mutex_lock(&a->mutex);
    pthread_mutex_lock(&b->mutex);
    temp = b->next;
    prev->next = b;
    b->next = a;
    a->next = temp;
    pthread_mutex_unlock(&prev->mutex);
    pthread_mutex_unlock(&a->mutex);
    pthread_mutex_unlock(&b->mutex);
}

void sortList(List **head){
    List *prev, *i, *j;
    prev=NULL;
    j=NULL;
    pthread_mutex_lock(&(*head)->mutex);
    i=(*head);
    pthread_mutex_unlock(&(*head)->mutex);

    while(i){
        pthread_mutex_lock(&(*head)->mutex);
        prev=(*head);
        j=(*head);
        pthread_mutex_unlock(&(*head)->mutex);

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
