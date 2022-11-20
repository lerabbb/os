#include <stdio.h>
#include <pthread.h>
#define IT_NUM 10
#define MUTEX_NUM 3

#define COMMON 0
#define PARENT 1
#define CHILD 2
pthread_mutex_t mutex[MUTEX_NUM];

void *task(void *args){
    pthread_mutex_lock(&mutex[CHILD]);
    for(int i=0; i<IT_NUM; i++){
        pthread_mutex_lock(&mutex[PARENT]);
        printf("child %d\n", i);
        pthread_mutex_unlock(&mutex[CHILD]);
        pthread_mutex_lock(&mutex[COMMON]);
        pthread_mutex_unlock(&mutex[PARENT]);
        pthread_mutex_lock(&mutex[CHILD]);
        pthread_mutex_unlock(&mutex[COMMON]);
    }
    pthread_mutex_unlock(&mutex[CHILD]);
}

int main(int argc, char **argv){
    pthread_t tid;

    pthread_mutexattr_t attr;
    if(pthread_mutexattr_init(&attr)){
        perror("ERROR: mutex attribute not initialized");
        return -1;
    }
    if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK)){
        perror("ERROR: mutex type not set");
        return -1;
    }

    for(int i=0; i<MUTEX_NUM; i++){
        if(pthread_mutex_init(&mutex[i], &attr)){
            perror("ERROR: mutex not initialized");
            return -1;
        }
    }

    pthread_mutex_lock(&mutex[PARENT]);
    if(pthread_create(&tid, NULL, task, NULL)){
        perror("ERROR: thread not created");
        return -1;
    }

    for(int i=0; i<IT_NUM; i++){
        printf("parent %d\n", i);
        pthread_mutex_lock(&mutex[COMMON]);
        pthread_mutex_unlock(&mutex[PARENT]);
        pthread_mutex_lock(&mutex[CHILD]);
        pthread_mutex_unlock(&mutex[COMMON]);
        pthread_mutex_lock(&mutex[PARENT]);
        pthread_mutex_unlock(&mutex[CHILD]);
    }
    pthread_mutex_unlock(&mutex[PARENT]);

    if(pthread_join(tid, NULL)){
        perror("thread not joined");
        return -1;
    }

    for(int i=0; i<MUTEX_NUM; i++){
        if(pthread_mutex_destroy(&mutex[i])){
            perror("ERROR: mutex not initialized");
            return -1;
        }
    }
    pthread_mutexattr_destroy(&attr);
    pthread_exit(0);
}

