#ifndef OS18_LIST_H
#define OS18_LIST_H

#endif //OS18_LIST_H

#include <pthread.h>
#define MAX_STR_LEN 400
#define MAX_BUF 80

typedef struct Node{
    char *buf;
    struct Node *next;
    pthread_mutex_t mutex;
} Node;

typedef struct List{
    Node *head;
    pthread_mutex_t mainMutex;
} List;

List* createList();
int destroyList(List **list);

int init(List **list);
int push(char *str, List **list);
int pop(List **list);
void printList(List **list);
void sortList(List **list);
