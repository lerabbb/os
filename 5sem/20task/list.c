#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int init(char *str, List **list){
    pthread_rwlock_wrlock(&(*list)->mainRwlock);
    (*list)->head = (Node*)malloc(sizeof(Node));

    if(pthread_rwlock_init(&(*list)->head->rwlock, NULL)){
        return 1;
    }

    (*list)->head->buf = (char*)malloc(sizeof(char)*MAX_BUF);
    strcpy((*list)->head->buf, str);
    (*list)->head->next=NULL;
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
    temp->next = (*list)->head;
    (*list)->head = temp;
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
    temp = (*list)->head;
    pthread_rwlock_unlock(&(*list)->mainRwlock);

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
        pthread_rwlock_wrlock(&(*list)->mainRwlock);
        pthread_rwlock_wrlock(&a->rwlock);
        pthread_rwlock_wrlock(&b->rwlock);
        temp = b->next;
        b->next = a;
        a->next = temp;
        (*list)->head = b;
        pthread_rwlock_unlock(&(*list)->mainRwlock);
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

void sortList(List **list){
    Node *i, *j, *prev;
    j = NULL;
    prev = NULL;
    pthread_rwlock_rdlock(&(*list)->mainRwlock);
    i = (*list)->head;
    pthread_rwlock_unlock(&(*list)->mainRwlock);

    while(i){
        pthread_rwlock_rdlock(&(*list)->mainRwlock);
        prev = (*list)->head;
        j = (*list)->head;
        pthread_rwlock_unlock(&(*list)->mainRwlock);

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
    if(pthread_rwlock_init(&list->mainRwlock, NULL)){
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

    if(pthread_rwlock_destroy(&(*list)->mainRwlock)){
        perror("Fail while destroying main rwlock");
        return 1;
    }
    return 0;
}
