#include <sys/types.h>
#include <stdio.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define IT_NUM 10
sem_t *sem1, *sem2;

int main(int argc, char **argv){
    pid_t pid;


    sem1 = sem_open("/lab16_sem1", O_CREAT, 0666, 1);
    if(sem1 == SEM_FAILED){
        perror("ERROR: sem1 not created");
        return -1;
    }
    sem2 = sem_open("/lab16_sem2", O_CREAT, 0666, 0);
    if(sem2 == SEM_FAILED) {
        perror("ERROR: sem2 not created");
        return -1;
    }

    pid = fork();
    if(pid == -1){
        perror("ERROR: new proc not created");
        return -1;
    }

    if(pid == 0){
        for(int i=0; i<IT_NUM; i++){
            sem_wait(sem1);
            printf("parent %d\n", i);
            sem_post(sem2);
        }
    } else{
        for(int i=0; i<IT_NUM; i++){
            sem_wait(sem2);
            printf("child %d\n", i);
            sem_post(sem1);
        }
    }
    wait(NULL);

    if(sem_close(sem1) == -1){
        perror("ERROR: sem1 not closed");
    }
    if(sem_close(sem2) == -1){
        perror("ERROR: sem2 not closed");
    }
    return 0;
}

