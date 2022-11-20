#ifndef OS17_LIST_H
#define OS17_LIST_H

#endif //OS17_LIST_H

#define MAX_STR_LEN 400
#define MAX_BUF 80

#include <pthread.h>

typedef struct List{
    char *buf;
    struct List *next;
    pthread_mutex_t mutex;
} List;

int init(List **head, char *str);
int push(char *str, List **head);
int pop(List **head);
int printList(List **head);
void sortList(List **head);
