#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define IT_NUM 200000000

typedef struct Args{
    int tnum;
    int start;
    double part_sum;
} Args;

void *func(void *args){
    Args *arg = (Args*) args;
    double temp = 0;

    for(long i = arg->start; i < IT_NUM; i+=arg->tnum) {
        temp += 1.0/(i*4.0 + 1.0);
        temp -= 1.0/(i*4.0 + 3.0);
    }

    arg->part_sum = temp;
    pthread_exit(arg);
}

int main(int argc, char **argv) {
    int tnum, err;
    long offset = 0;
    double pi = 0;

    pthread_t *tid;
    Args *args;

    if(argc !=2){
        perror("Invalid arguments\n");
        return -1;
    } else{
        tnum = atoi(argv[1]);
    }
    if(tnum <= 0){
        perror("Invalid arguments\n");
        return -1;
    }

    tid = (pthread_t*)malloc(sizeof(pthread_t)*tnum);
    args = (Args*)malloc(sizeof (Args)*tnum);

    for(int i=0; i < tnum; i++){
        args[i].start = i;
        args[i].tnum = tnum;
        args[i].part_sum = 0;

        err = pthread_create(&tid[i], NULL, func, (void*)&args[i]); //change args
        if(err){
            perror("Error: thread not created");
            return -1;
        }
    }

    for(int i=0; i < tnum; i++){
        //Args result;
        if(pthread_join(tid[i], (void*)&args[i])){
            perror("Error: thread not joined");
            return -1;
        }
        printf("thread %d | part_sum = %.15g\n", i, args[i].part_sum);
        pi += args[i].part_sum;
    }

    pi *= 4.0;
    printf("pi done = %.15g \n", pi);

    free(tid);
    free(args);
    return 0;
}

