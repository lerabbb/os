#ifndef OS18_LIST_H
#define OS18_LIST_H

#endif //OS18_LIST_H

#include <pthread.h>
#define MAX_STR_LEN 400
#define MAX_BUF 80

typedef struct List{
    char *buf;
    struct List *next;
    pthread_mutex_t mutex;
} List;

int init(pthread_mutex_t *mainMutex, char *str, List **head);
int push(pthread_mutex_t *mainMutex, char *str, List **head);
int pop(pthread_mutex_t *mainMutex, List **head);
void printList(pthread_mutex_t *mainMutex, List **head);
void sortList(pthread_mutex_t *mainMutex, List **head);
