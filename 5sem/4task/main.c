#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void *task(void *args){
        while(1){
                printf("child ");
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
        sleep(2);
        if(pthread_cancel(tid)){
                perror("ERROR: thread not cancelled\n");
                return -1;
        }

        printf("\n2 sec left, child thread cancelled\n");
        return 0;
}

