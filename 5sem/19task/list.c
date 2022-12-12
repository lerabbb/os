#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#define SWAP_PERIOD 1000000

int init(List **list){
    pthread_mutex_lock(&(*list)->mainMutex);
    (*list)->head = (Node*)malloc(sizeof(Node));

    if(pthread_mutex_init(&(*list)->head->mutex, NULL)){
        return 1;
    }

    (*list)->head->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    (*list)->head->next = NULL;
    pthread_mutex_unlock(&(*list)->mainMutex);
    return 0;
}

int push(char *str, List **list){
    Node *temp = (Node*) malloc(sizeof(Node));

    if(pthread_mutex_init(&temp->mutex, NULL)){
        return 1;
    }

    temp->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy(temp->buf, str);

    pthread_mutex_lock(&(*list)->mainMutex);
    temp->next = (*list)->head->next;
    (*list)->head->next = temp;
    pthread_mutex_unlock(&(*list)->mainMutex);
    return 0;
}

int pop(List **list){
    Node *temp =NULL;

    pthread_mutex_lock(&(*list)->mainMutex);
    if((*list) == NULL || (*list)->head == NULL){
        return 1;
    }
    temp = (*list)->head;
    (*list)->head = (*list)->head->next;
    pthread_mutex_unlock(&(*list)->mainMutex);

    if(pthread_mutex_destroy(&temp->mutex)){
        return 1;
    }
    free(temp->buf);
    free(temp);
    return 0;
}

void printList(List **list){
    Node *temp = NULL;
    pthread_mutex_lock(&(*list)->mainMutex);
    temp = (*list)->head->next;
    pthread_mutex_unlock(&(*list)->mainMutex);

    printf("List: ");
    pthread_mutex_lock(&temp->mutex);
    while(temp){
        printf("\'%s\' -> ", temp->buf);
        if(temp->next){
            pthread_mutex_lock(&temp->next->mutex);
        }
        pthread_mutex_unlock(&temp->mutex);
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
        pthread_mutex_lock(&(*list)->mainMutex);
        prev = (*list)->head;
        pthread_mutex_unlock(&(*list)->mainMutex);

        pthread_mutex_lock(&prev->mutex);
        i = (*list)->head->next;
        if(i){
            pthread_mutex_lock(&i->mutex);
            j = i->next;
            while(j){
                pthread_mutex_lock(&j->mutex);
                if(strcmp(i->buf, j->buf) > 0){
                    flag = 1;
                    printf("[swap] i = %s and j = %s\n", i->buf, j->buf);
                    swap(&prev, &i, &j);
                    usleep(SWAP_PERIOD);
                }
                pthread_mutex_unlock(&prev->mutex);
                prev = i;
                i = j;
                j = j->next;
            }
            pthread_mutex_unlock(&i->mutex);
        }
        pthread_mutex_unlock(&prev->mutex);
    }
}

List* createList(){
    List *list = (List*)malloc(sizeof (List));
    if(pthread_mutex_init(&list->mainMutex, NULL)){
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

    if(pthread_mutex_destroy(&(*list)->mainMutex)){
        perror("Fail while destroying mutex");
        return 1;
    }
    return 0;
}
