#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "list.h"

#define PERIOD 5000000

List *head=NULL;

void routine(void *arg){
    List *temp=head;
    while(temp){
        pthread_mutex_unlock(&temp->mutex);
        temp=temp->next;
    }
}

void *child_func(void *args){
    pthread_cleanup_push(routine, NULL);
    int err;
    while(1) {
        usleep(PERIOD);
        sortList(&head);
        printf("list sorted by child\n");
    }
    pthread_cleanup_pop(1);
}

void parent_func(){
    char* buf = (char*) malloc(sizeof(char)*MAX_STR_LEN);
    if(fgets(buf, MAX_BUF, stdin) == NULL){
        printf("quit\n");
        free(buf);
        return;
    }
    if(init(&head,buf)){
        printf("quit\n");
        free(buf);
        return;
    }
    buf[strcspn(buf, "\n")] = 0;
    if(strcmp(buf, "end")==0){
        printf("quit\n");
        free(buf);
        return;
    }

    while(1){
        if(fgets(buf, MAX_BUF, stdin) == NULL){
            break;
        }
        buf[strcspn(buf, "\n")] = 0;
        if(strcmp(buf, "end")==0){
            break;
        }
        if(strcmp(buf, "")==0) {
            if(printList(&head)){
                break;
            }
        }
        else {
            strcat(buf, "\0");
            if(push(buf, &head)){
                break;
            }
            printf("new node added\n");
        }
    }
    printf("quit\n");
    free(buf);
}

int main() {
    pthread_t tid;

    if(pthread_create(&tid, NULL, child_func, NULL)){
        perror("thread not created");
        return -1;
    }
    printf("Enter \'end\' to exit\n");
    parent_func();

    if(pthread_cancel(tid)){
        perror("child thread not cancelled");
        return -1;
    }
    printf("child cancelled\n");

    if(printList(&head)){
        return 1;
    }
    while(head){
        if(pop(&head)){
            perror("fail while list clearing");
            return -1;
        }
    }

    return 0;
}
