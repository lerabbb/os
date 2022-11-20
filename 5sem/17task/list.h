#ifndef OS17_LIST_H
#define OS17_LIST_H

#endif //OS17_LIST_H

#define MAX_STR_LEN 400
#define MAX_BUF 80

typedef struct List{
    char *buf;
    struct List *next;
    struct List *prev;
} List;

void push(char *str, List **head);
int pop(List **head);
void printList(List *head);
int sortList(List **head);
