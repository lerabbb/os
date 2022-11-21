#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "list.h"

#define PERIOD 5000000

List *head=NULL;
pthread_mutex_t mutex;

void *child_func(void *args){
    int err;
    while(1) {
        usleep(PERIOD);
        pthread_mutex_lock(&mutex);
        err=sortList(&head);
        pthread_mutex_unlock(&mutex);
        if(err){
            perror("Fail while list sorting");
            exit(1);
        }
    }
}

void parent_func(){
    char* buf = (char*) malloc(sizeof(char)*MAX_STR_LEN);

    while(1){
        if(fgets(buf, MAX_BUF, stdin) == NULL){
            break;
        }
        buf[strcspn(buf, "\n")] = 0;
        if(strcmp(buf, "end")==0){
            break;
        }
        if(strcmp(buf, "")==0) {
            printf("\n");
            pthread_mutex_lock(&mutex);
            printList(head);
            pthread_mutex_unlock(&mutex);
        }
        else {
            pthread_mutex_lock(&mutex);
            strcat(buf, "\0");
            push(buf, &head);
            pthread_mutex_unlock(&mutex);
        }
    }
    free(buf);
}

int main() {
    pthread_t tid;
    if(pthread_mutex_init(&mutex, NULL)){
        perror("mutex not initialized");
        return -1;
    }
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
    if(pthread_mutex_destroy(&mutex)){
        perror("mutex not destroyed");
        return -1;
    }

    printList(head);
    while(head){
        if(pop(&head)){
            perror("fail while list clearing");
            return -1;
        }
    }

    return 0;
}


