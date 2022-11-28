#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

Node *prev = NULL;
int init(char *str, List **list){
    pthread_mutex_lock(&(*list)->mainMutex);
    (*list)->head = (Node*)malloc(sizeof(Node));

    if(pthread_mutex_init(&(*list)->head->mutex, NULL)){
        return 1;
    }

    (*list)->head->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy((*list)->head->buf, str);
    (*list)->head->next=NULL;
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
    temp->next = (*list)->head;
    (*list)->head = temp;
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
    temp = (*list)->head;
    pthread_mutex_unlock(&(*list)->mainMutex);

    printf("List: ");
    while(temp){
        printf("\'%s\' -> ", temp->buf);
        temp=temp->next;
    }
    printf("\n");
}

void swap(List **list, Node *prev, Node *a, Node *b){
    Node *temp = NULL;

    if(a == prev){
        pthread_mutex_lock(&(*list)->mainMutex);
        pthread_mutex_lock(&a->mutex);
        pthread_mutex_lock(&b->mutex);
        temp = b->next;
        b->next = a;
        a->next = temp;
        (*list)->head = b;
        pthread_mutex_unlock(&(*list)->mainMutex);
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

void sortList(List **list){
    Node *i, *j;
    j = NULL;
    pthread_mutex_lock(&(*list)->mainMutex);
    i = (*list)->head;
    pthread_mutex_unlock(&(*list)->mainMutex);

    while(i){
        pthread_mutex_lock(&(*list)->mainMutex);
        prev = (*list)->head;
        j = (*list)->head;
        pthread_mutex_unlock(&(*list)->mainMutex);

        while(j != i && j!=NULL && j->next!=NULL){
            if(strcmp(j->buf, j->next->buf)>0){
                swap(list,  prev, j, j->next);
	    }
            prev = j;
            j = j->next;
        }

        i=i->next;
    }
}

List* createList(){
    List *list = (List*)malloc(sizeof (List));
    if(pthread_mutex_init(&list->mainMutex, NULL)){
        return NULL;
    }
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
