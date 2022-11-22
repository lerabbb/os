#ifndef OS21_LIST_H
#define OS21_LIST_H

#endif //OS21_LIST_H

#include <pthread.h>
#define MAX_STR_LEN 400
#define MAX_BUF 80

typedef struct List{
    char *buf;
    struct List *next;
    pthread_rwlock_t rwlock;
} List;

int init(pthread_rwlock_t *mainRwlock, char *str, List **head);
int push(pthread_rwlock_t *mainRwlock, char *str, List **head);
int pop(pthread_rwlock_t *mainRwlock, List **head);
void printList(pthread_rwlock_t *mainRwlock, List **head);
void sortList(pthread_rwlock_t *mainRwlock, List **head);
