#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int init(pthread_mutex_t *mainMutex, char *str, List **head){
    pthread_mutex_lock(mainMutex);
    (*head) = (List*)malloc(sizeof(List));

    if(pthread_mutex_init(&(*head)->mutex, NULL)){
        return 1;
    }

    (*head)->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy((*head)->buf, str);
    (*head)->next=NULL;
    pthread_mutex_unlock(mainMutex);
    return 0;
}

int push(pthread_mutex_t *mainMutex, char *str, List **head){
    List *temp = (List*) malloc(sizeof(List));

    if(pthread_mutex_init(&temp->mutex, NULL)){
        return 1;
    }

    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);

    pthread_mutex_lock(mainMutex);
    temp->next = (*head);
    (*head) = temp;
    pthread_mutex_unlock(mainMutex);
    return 0;
}

int pop(pthread_mutex_t *mainMutex, List **head){
    List *temp =NULL;

    pthread_mutex_lock(mainMutex);
    if((*head) == NULL){
        return 1;
    }
    temp = (*head);
    (*head)=(*head)->next;
    pthread_mutex_unlock(mainMutex);

    if(pthread_mutex_destroy(&temp->mutex)){
        return 1;
    }
    free(temp->buf);
    free(temp);
    return 0;
}

void printList(pthread_mutex_t *mainMutex, List **head){
    List *temp = NULL;
    pthread_mutex_lock(mainMutex);
    temp = (*head);
    pthread_mutex_unlock(mainMutex);

    printf("List: ");
    while(temp){
        printf("\'%s\' -> ", temp->buf);
        temp=temp->next;
    }
    printf("\n");
}

void swap(pthread_mutex_t *mainMutex, List **head, List *prev, List *a, List *b){
    List *temp = NULL;

    if(a == prev){
        pthread_mutex_lock(&a->mutex);
        pthread_mutex_lock(&b->mutex);
        temp = b->next;
        b->next = a;
        a->next = temp;
        pthread_mutex_lock(mainMutex);
        (*head) = b;
        pthread_mutex_unlock(mainMutex);
        pthread_mutex_unlock(&a->mutex);
        pthread_mutex_unlock(&b->mutex);
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

void sortList(pthread_mutex_t *mainMutex, List **head){
    List *prev, *i, *j;
    prev=NULL;
    j=NULL;
    pthread_mutex_lock(mainMutex);
    i=(*head);
    pthread_mutex_unlock(mainMutex);

    while(i){
        pthread_mutex_lock(mainMutex);
        prev=(*head);
        j=(*head);
        pthread_mutex_unlock(mainMutex);

        while(j != i && j!=NULL && j->next!=NULL){
            if(strcmp(j->buf, j->next->buf)>0){
                swap(mainMutex, head, prev, j, j->next);
            }
            prev = j;
            j = j->next;
        }

        i=i->next;
    }
}
