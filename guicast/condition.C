#include "condition.h"

#include <errno.h>
#include <sys/time.h>

Condition::Condition(int init_value)
{
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, NULL);
	value = init_value;
}

Condition:: ~Condition()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}


void Condition::lock()
{
    pthread_mutex_lock(&mutex);
    while(value <= 0) pthread_cond_wait(&cond, &mutex);
	value--;
    pthread_mutex_unlock(&mutex);
}

void Condition::unlock()
{
    pthread_mutex_lock(&mutex);
    value++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

int Condition::timed_lock(int milliseconds)
{
    struct timeval now;
    struct timespec timeout;
    int result = 0;

    pthread_mutex_lock(&mutex);
    gettimeofday(&now, 0);
    timeout.tv_sec = now.tv_sec + milliseconds / 1000000;
    timeout.tv_nsec = now.tv_usec * 1000 + (milliseconds % 1000000) * 1000;

    while(value <= 0 && result != ETIMEDOUT)
	{
		result = pthread_cond_timedwait(&cond, &mutex, &timeout);
    }

    if(result == ETIMEDOUT) 
	{
		result = 1;
    } 
	else 
	{
		value--;
		result = 0;
    }
    pthread_mutex_unlock(&mutex);
	return result;
}


