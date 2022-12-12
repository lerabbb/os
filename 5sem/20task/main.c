#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "list.h"

#define PERIOD 5000000
#define THREAD_NUM 4

List *list=NULL;

void *child_func(void *args){
    while(1) {
        usleep(PERIOD);
        sortList(&list);
    }
}

void parent_func(){
    char* buf = (char*) malloc(sizeof(char)*MAX_STR_LEN);

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
            printList(&list);
        }
        else {
            strcat(buf, "\0");

            if(push(buf, &list)){
                perror("Fail while pushing node");
                break;
            }
        }
    }
    free(buf);
}

int main() {
    pthread_t tid[THREAD_NUM];
    list = createList();
    if(list == NULL){
        perror("Fail while creating list");
        return -1;
    }
    for(int i=0; i < THREAD_NUM; i++){
        if(pthread_create(&tid[i], NULL, child_func, NULL)){
            perror("thread not created");
            return -1;
        }
    }
    printf("Enter \'end\' to exit\n");
    parent_func();

    for(int i=0; i<THREAD_NUM; i++) {
        if (pthread_cancel(tid[i])) {
            perror("child thread not cancelled");
            return -1;
        }
    }

    printList(&list);
    if(destroyList(&list)){
        perror("Fail while destroying list");
        return -1;
    }
    return 0;
}
