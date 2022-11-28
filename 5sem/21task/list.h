#ifndef OS21_LIST_H
#define OS21_LIST_H

#endif //OS21_LIST_H

#include <pthread.h>
#define MAX_STR_LEN 400
#define MAX_BUF 80

typedef struct Node{
    char *buf;
    struct Node *next;
    pthread_rwlock_t rwlock;
} Node;

typedef struct List{
    Node *head;
    pthread_rwlock_t mainRwlock;
} List;

List* createList();
int destroyList(List **list);

int init(char *str, List **list);
int push(char *str, List **list);
int pop(List **list);
void printList(List **list);
void sortList(List **list);
