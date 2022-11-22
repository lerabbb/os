#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "list.h"

#define PERIOD 5000000

List *head=NULL;
pthread_mutex_t mainMutex;

void *child_func(void *args){
    while(1) {
        usleep(PERIOD);
        sortList(&mainMutex, &head);
    }
}

void parent_func(){
    char* buf = (char*) malloc(sizeof(char)*MAX_STR_LEN);

    //initialize list
    if(fgets(buf, MAX_BUF, stdin) == NULL){
        perror("Fail while reading text");
        free(buf);
        return;
    }
    if(strcmp(buf, "end\n")==0){
        free(buf);
        return;
    }
    buf[strcspn(buf, "\n")] = 0;
    if(init(&mainMutex, buf, &head)){
        perror("Fail while initializing list");
        free(buf);
        return;
    }

    while(1){
        if(fgets(buf, MAX_BUF, stdin) == NULL){
            break;
        }
        if(strcmp(buf, "end\n")==0){
            break;
        }
        buf[strcspn(buf, "\n")] = 0;
        if(strcmp(buf, "")==0) {
            printf("\n");
            printList(&mainMutex, &head);
        }
        else {
            strcat(buf, "\0");

            if(push(&mainMutex, buf, &head)){
                perror("Fail while pushing node");
                break;
            }
        }
    }
    free(buf);
}

int main() {
    pthread_t tid;
    if(pthread_mutex_init(&mainMutex, NULL)){
        perror("Fail while initializing main mutex");
        return 1;
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

    printList(&mainMutex, &head);
    while(head){
        if(pop(&mainMutex, &head)){
            perror("fail while list clearing");
            return -1;
        }
    }

    if(pthread_mutex_destroy(&mainMutex)){
        perror("Fail while destroying mutex");
        return 1;
    }

    return 0;
}


