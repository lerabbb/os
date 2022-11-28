#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define SWAP_PERIOD 1000000

int init(pthread_rwlock_t *mainRwlock, char *str, List **head){
    pthread_rwlock_rdlock(mainRwlock);
    (*head) = (List*)malloc(sizeof(List));

    if(pthread_rwlock_init(&(*head)->rwlock, NULL)){
        return 1;
    }

    (*head)->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy((*head)->buf, str);
    (*head)->next=NULL;
    pthread_rwlock_unlock(mainRwlock);
    return 0;
}

int push(pthread_rwlock_t *mainRwlock, char *str, List **head){
    List *temp = (List*) malloc(sizeof(List));

    if(pthread_rwlock_init(&temp->rwlock, NULL)){
        return 1;
    }

    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);

    pthread_rwlock_wrlock(mainRwlock);
    temp->next = (*head);
    (*head) = temp;
    pthread_rwlock_unlock(mainRwlock);
    return 0;
}

int pop(pthread_rwlock_t *mainRwlock, List **head){
    List *temp =NULL;

    pthread_rwlock_wrlock(mainRwlock);
    if((*head) == NULL){
        return 1;
    }
    temp = (*head);
    (*head)=(*head)->next;
    pthread_rwlock_unlock(mainRwlock);

    if(pthread_rwlock_destroy(&temp->rwlock)){
        return 1;
    }
    free(temp->buf);
    free(temp);
    return 0;
}

void printList(pthread_rwlock_t *mainRwlock, List **head){
    List *temp = NULL;
    pthread_rwlock_rdlock(mainRwlock);
    temp = (*head);
    pthread_rwlock_unlock(mainRwlock);

    printf("List: ");
    while(temp){
        printf("\'%s\' -> ", temp->buf);
        temp=temp->next;
    }
    printf("\n");
}

void swap(pthread_rwlock_t *mainRwlock, List **head, List *prev, List *a, List *b){
    List *temp = NULL;

    if(a == prev){
        pthread_rwlock_wrlock(mainRwlock);
	pthread_rwlock_wrlock(&a->rwlock);
        pthread_rwlock_wrlock(&b->rwlock);
        temp = b->next;
        b->next = a;
        a->next = temp;
        (*head) = b;
        pthread_rwlock_unlock(mainRwlock);
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

void sortList(pthread_rwlock_t *mainRwlock, List **head){
    List *prev, *i, *j;
    prev=NULL;
    j=NULL;
    pthread_rwlock_rdlock(mainRwlock);
    i=(*head);
    pthread_rwlock_unlock(mainRwlock);

    printf("\nStart sorting:\n");
    while(i){
        pthread_rwlock_rdlock(mainRwlock);
        prev=(*head);
        j=(*head);
        pthread_rwlock_unlock(mainRwlock);

        while(j != i && j!=NULL && j->next!=NULL){
            usleep(SWAP_PERIOD);
            printList(mainRwlock, head);

            if(strcmp(j->buf, j->next->buf)>0){
                swap(mainRwlock, head, prev, j, j->next);
            }
            prev = j;
            j = j->next;
        }

        i=i->next;
    }
    printf("Stop sorting\n");
}
