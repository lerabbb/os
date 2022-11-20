#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define IT_NUM 10
#define PSHARED_FLAG 0
#define SEM_NUM 2
sem_t sem[SEM_NUM];

void *task(void *args){
    for(int i=0; i<IT_NUM; i++){
        sem_wait(&sem[1]);
        printf("child %d\n", i);
        sem_post(&sem[0]);
    }
}

int main(int argc, char **argv){
    pthread_t tid;

    for(int i=0; i<SEM_NUM; i++){
        if(sem_init(&sem[i], PSHARED_FLAG, i) == -1){
            perror("ERROR: semaphore not initialized");
            return -1;
        }
    }

    if(pthread_create(&tid, NULL, task, NULL)){
        perror("ERROR: thread not created");
        return -1;
    }

    for(int i=0; i<IT_NUM; i++){
        sem_wait(&sem[0]);
        printf("parent %d\n", i);
        sem_post(&sem[1]);
    }

    if(pthread_join(tid, NULL)){
        perror("thread not joined");
        return -1;
    }
    for(int i=0; i<SEM_NUM; i++){
        if(sem_destroy(&sem[i]) == -1){
            perror("ERROR: semaphore not destroyed");
            return -1;
        }
    }

    pthread_exit(0);
}

