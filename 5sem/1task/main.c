#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void *task(void *args){
        for(int i=0; i<10; i++){
                printf("child %d\n", i);
        }
}

int main(int argc, char **argv){
        int err;
        pthread_t tid;
        err = pthread_create(&tid, NULL, task, NULL);
        if(err){
                perror("ERROR: thread not created");
                return -1;
        }
        for(int i=0; i<10; i++){
                printf("parent %d\n", i);
        }
        pthread_exit(0);
}

