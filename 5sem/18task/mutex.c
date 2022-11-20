#include "mutex.h"
#include <stdio.h>

int init_mutex(pthread_mutex_t *mutex) {
    //pthread_mutexattr_t attr;
    int err;
    /*if((err=pthread_mutexattr_init(&attr))){
        return err;
    }
    if((err=pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK))){
        return err;
    }*/
    if((err=pthread_mutex_init(mutex, NULL))){
        return err;
    }
    return 0;
}

int destroy(pthread_mutex_t *mutex) {
    int err;
    if((err=pthread_mutex_destroy(mutex))){
        return err;
    }
    return 0;
}

int lock(pthread_mutex_t *mutex) {
    int err = pthread_mutex_lock(mutex);
    if(err){
        printf("error while mutex locking: %d", err);
        return 1;
    }

    return 0;
}

int unlock(pthread_mutex_t *mutex) {
    int err = pthread_mutex_unlock(mutex);
    if(err){
        printf("error while mutex locking: %d", err);
        return 1;
    }
    return 0;
}

