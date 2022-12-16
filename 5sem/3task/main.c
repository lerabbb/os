#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#define NUM_THREADS 4
#define STR_NUM 30
#define MAX_LEN 100

typedef struct Args{
        char str[STR_NUM][MAX_LEN];
} Args;

void *task(void *args){
        Args *arg = (Args*) args;

        for(int i=0; i<STR_NUM; i++){
		usleep(200000);
                printf("%s", arg->str[i]);
        }
}

int main(int argc, char **argv){
        int err;
        pthread_t threads[NUM_THREADS];
        Args args[NUM_THREADS];

        for(int i=0; i<NUM_THREADS; i++){
                for(int j=0; j<STR_NUM; j++)
                        snprintf(args[i].str[j], MAX_LEN, "Thread %d - line %d\n", i, j);
        }

        for(int i=0; i<NUM_THREADS; i++){
                err = pthread_create(&threads[i], NULL, task, (void*)&args[i]);
                if(err){
                        perror("ERROR: thread not created");
                        return -1;
                }
        }

        for(int i=0; i<NUM_THREADS; i++){
                if(pthread_join(threads[i], NULL) != 0){
                        perror("ERROR: thread not joined");
                        return -1;
                }
        }
        return 0;
}
