#include "mutex.h"

Mutex::Mutex()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
//	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_FAST_NP);
	pthread_mutex_init(&mutex, &attr);
}

Mutex::~Mutex()
{
//printf("Mutex::~Mutex 1\n");
	pthread_mutex_destroy(&mutex);
//printf("Mutex::~Mutex 2\n");
}
	
int Mutex::lock()
{
	if(pthread_mutex_lock(&mutex)) perror("Mutex::lock");
	return 0;
}

int Mutex::unlock()
{
	if(pthread_mutex_unlock(&mutex)) perror("Mutex::unlock");
	return 0;
}

int Mutex::trylock()
{
	return pthread_mutex_trylock(&mutex);
}

int Mutex::reset()
{
	pthread_mutex_destroy(&mutex);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&mutex, &attr);
	return 0;
}
