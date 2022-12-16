#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define MAX_LEN 100
#define MAX_STR_NUM 100
#define DELAY 50000

void* sleppsort(void *args){
    char* str = (char*)args;
    usleep(strlen(str)*DELAY);
    printf("%s", str);
}

void free_mem(char **buf, int N){
    for(int i=0; i<N; i++){
        free(buf[i]);
    }
    free(buf);
}

int main() {
    pthread_t tid[MAX_STR_NUM];
    char **buf = (char**)malloc(sizeof(char*)*MAX_STR_NUM);
    int i, N;

    for(i=0; i<MAX_STR_NUM; i++){
        buf[i] = (char*)malloc(sizeof(char)*MAX_LEN);

        if(fgets(buf[i], MAX_LEN, stdin) == NULL){
            free(buf[i]);
            break;
        }
        if(strcmp(buf[i], "\n") == 0){
            i++;
            break;
        }
    }
    N=i;

    printf("sorted strings:\n");
    for(i=0; i<N; i++){
        if(pthread_create(&tid[i], NULL, sleppsort, buf[i])){
            free_mem(buf, N);
            perror("thread wasn't created");
            return -1;
        }
    }
    for(i=0; i<N; i++){
        if(pthread_join(tid[i], NULL)){
            free_mem(buf, N);
            perror("thread wasn't joined");
            return -1;
        }
    }

    free_mem(buf, N);
    return 0;
}

