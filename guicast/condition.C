#ifndef NO_GUICAST
#include "bcsignals.h"
#endif
#include "condition.h"

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

Condition::Condition(int init_value, char *title, int is_binary)
{
	this->is_binary = is_binary;
	this->title = title;
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, NULL);
	this->value = this->init_value = init_value;
}

Condition:: ~Condition()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
#ifndef NO_GUICAST
	UNSET_ALL_LOCKS(this);
#endif
}

void Condition::reset()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, NULL);
	value = init_value;
}

void Condition::lock(char *location)
{
#ifndef NO_GUICAST
	SET_LOCK(this, title, location);
#endif
    pthread_mutex_lock(&mutex);
    while(value <= 0) pthread_cond_wait(&cond, &mutex);
#ifndef NO_GUICAST
	UNSET_LOCK2
#endif
	if(is_binary)
		value = 0;
	else
		value--;
    pthread_mutex_unlock(&mutex);
}

void Condition::unlock()
{
// The lock trace is created and removed by the acquirer
//#ifndef NO_GUICAST
//	UNSET_LOCK(this);
//#endif
    pthread_mutex_lock(&mutex);
    if(is_binary)
		value = 1;
	else
		value++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

int Condition::timed_lock(int microseconds, char *location)
{
    struct timeval now;
    struct timespec timeout;
    int result = 0;

#ifndef NO_GUICAST
	SET_LOCK(this, title, location);
#endif
    pthread_mutex_lock(&mutex);
    gettimeofday(&now, 0);
    timeout.tv_sec = now.tv_sec + microseconds / 1000000;
    timeout.tv_nsec = now.tv_usec * 1000 + (microseconds % 1000000) * 1000;

    while(value <= 0 && result != ETIMEDOUT)
	{
		result = pthread_cond_timedwait(&cond, &mutex, &timeout);
    }

    if(result == ETIMEDOUT) 
	{
//printf("Condition::timed_lock 1 %s %s\n", title, location);
#ifndef NO_GUICAST
		UNSET_LOCK2
#endif
		result = 1;
    } 
	else 
	{
//printf("Condition::timed_lock 2 %s %s\n", title, location);
#ifndef NO_GUICAST
		UNSET_LOCK2
#endif
		if(is_binary)
			value = 0;
		else
			value--;
		result = 0;
    }
    pthread_mutex_unlock(&mutex);
	return result;
}


int Condition::get_value()
{
	return value;
}
