#include "bcsignals.h"
#include "mutex.h"

Mutex::Mutex(char *title)
{
	this->title = title;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
//	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	pthread_mutex_init(&mutex, &attr);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&mutex);
	UNSET_ALL_LOCKS(this);
}
	
int Mutex::lock(char *location)
{
	SET_LOCK(this, title, location);
	if(pthread_mutex_lock(&mutex)) perror("Mutex::lock");
	SET_LOCK2
	return 0;
}

int Mutex::unlock()
{
	UNSET_LOCK(this);
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
	UNSET_ALL_LOCKS(this)
	return 0;
}
