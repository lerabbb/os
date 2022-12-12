#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#define SWAP_PERIOD 1000000

int init(List **list){
    pthread_rwlock_wrlock(&(*list)->mainRwlock);
    (*list)->head = (Node*)malloc(sizeof(Node));

    if(pthread_rwlock_init(&(*list)->head->rwlock, NULL)){
        return 1;
    }

    (*list)->head->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    (*list)->head->next = NULL;
    pthread_rwlock_unlock(&(*list)->mainRwlock);
    return 0;
}

int push(char *str, List **list){
    Node *temp = (Node*) malloc(sizeof(Node));

    if(pthread_rwlock_init(&temp->rwlock, NULL)){
        return 1;
    }

    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);

    pthread_rwlock_wrlock(&(*list)->mainRwlock);
    temp->next = (*list)->head->next;
    (*list)->head->next = temp;
    pthread_rwlock_unlock(&(*list)->mainRwlock);
    return 0;
}

int pop(List **list){
    Node *temp =NULL;

    pthread_rwlock_wrlock(&(*list)->mainRwlock);
    if((*list) == NULL || (*list)->head == NULL){
        return 1;
    }
    temp = (*list)->head;
    (*list)->head = (*list)->head->next;
    pthread_rwlock_unlock(&(*list)->mainRwlock);

    if(pthread_rwlock_destroy(&temp->rwlock)){
        return 1;
    }
    free(temp->buf);
    free(temp);
    return 0;
}

void printList(List **list){
    Node *temp = NULL;
    pthread_rwlock_rdlock(&(*list)->mainRwlock);
    temp = (*list)->head->next;
    pthread_rwlock_unlock(&(*list)->mainRwlock);

    printf("List: ");
    pthread_rwlock_rdlock(&temp->rwlock);
    while(temp){
        printf("\'%s\' -> ", temp->buf);
        if(temp->next){
            pthread_rwlock_rdlock(&temp->next->rwlock);
        }
        pthread_rwlock_unlock(&temp->rwlock);
        temp = temp->next;
    }
    printf("\n");
}

void swap(Node **prev, Node **a, Node **b){
    (*prev)->next = (*b);
    (*a)->next = (*b)->next;
    (*b)->next = (*a);
    (*a) = (*b);
    (*b) = (*a)->next;
}

void sortList(List **list){
    int flag = 1;
    Node *prev, *i, *j;
    prev = NULL;
    j = NULL;
    i = NULL;

    while(flag){
        flag = 0;
        pthread_rwlock_rdlock(&(*list)->mainRwlock);
        prev = (*list)->head;
        pthread_rwlock_unlock(&(*list)->mainRwlock);

        pthread_rwlock_wrlock(&prev->rwlock);
        i = (*list)->head->next;
        if(i){
            pthread_rwlock_wrlock(&i->rwlock);
            j = i->next;
            while(j){
                pthread_rwlock_wrlock(&j->rwlock);
                if(strcmp(i->buf, j->buf) > 0){
                    flag = 1;
                    printf("[swap] i = %s and j = %s\n", i->buf, j->buf);
                    swap(&prev, &i, &j);
                    usleep(SWAP_PERIOD);
                }
                pthread_rwlock_unlock(&prev->rwlock);
                prev = i;
                i = j;
                j = j->next;
            }
            pthread_rwlock_unlock(&i->rwlock);
        }
        pthread_rwlock_unlock(&prev->rwlock);
    }
}

List* createList(){
    List *list = (List*)malloc(sizeof (List));
    if(pthread_rwlock_init(&list->mainRwlock, NULL)){
        return NULL;
    }
    init(&list);

    return list;
}
int destroyList(List **list){
    while((*list)->head){
        if(pop(list)){
            perror("fail while list clearing");
            return -1;
        }
    }

    if(pthread_rwlock_destroy(&(*list)->mainRwlock)){
        perror("Fail while destroying rwlock");
        return 1;
    }
    return 0;
}
