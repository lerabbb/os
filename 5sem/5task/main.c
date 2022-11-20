#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void routine(void *arg){
        printf("\nChild cancelled by parent\n");
}

void *task(void *args){
        pthread_cleanup_push(routine, NULL);
        printf("Child started\n");
        while(1){
                printf("child ");
        }
        pthread_cleanup_pop(1);
}

int main(int argc, char **argv){
        int err;
        pthread_t tid;

        err = pthread_create(&tid, NULL, task, NULL);
        if(err){
                perror("ERROR: thread not created");
                return -1;
        }
        sleep(2);
        err = pthread_cancel(tid);
        if(err){
                perror("ERROR: thread not cancelled\n");
                return -1;
        }

        pthread_exit(0);
}

