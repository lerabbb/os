#ifndef OS17_MUTEX_H
#define OS17_MUTEX_H

#endif //OS17_MUTEX_H

#include <pthread.h>

int init_mutex(pthread_mutex_t *mutex);
int destroy(pthread_mutex_t *mutex);
int lock(pthread_mutex_t *mutex);
int unlock(pthread_mutex_t *mutex);
