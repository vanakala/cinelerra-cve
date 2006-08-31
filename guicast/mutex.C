#ifndef NO_GUICAST
#include "bcsignals.h"
#endif
#include "mutex.h"

Mutex::Mutex(char *title, int recursive)
{
	this->title = title;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&mutex, &attr);
	pthread_mutex_init(&recursive_lock, &attr);
	count = 0;
	this->recursive = recursive;
	thread_id = 0;
	thread_id_valid = 0;
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&recursive_lock);
#ifndef NO_GUICAST
	UNSET_ALL_LOCKS(this);
#endif
}
	
int Mutex::lock(char *location)
{
// Test recursive owner and give up if we already own it
	if(recursive)
	{
		pthread_mutex_lock(&recursive_lock);
		if(thread_id_valid && pthread_self() == thread_id) 
		{
			count++;
			pthread_mutex_unlock(&recursive_lock);
			return 0;
		}
		pthread_mutex_unlock(&recursive_lock);
	}


#ifndef NO_GUICAST
	SET_LOCK(this, title, location);
#endif
	if(pthread_mutex_lock(&mutex)) perror("Mutex::lock");



// Update recursive status for the first lock
	if(recursive)
	{
		pthread_mutex_lock(&recursive_lock);
		count = 1;
		thread_id = pthread_self();
		thread_id_valid = 1;
		pthread_mutex_unlock(&recursive_lock);
	}
	else
	{
		count = 1;
	}


#ifndef NO_GUICAST
	SET_LOCK2
#endif
	return 0;
}

int Mutex::unlock()
{
// Remove from recursive status
	if(recursive)
	{
		pthread_mutex_lock(&recursive_lock);
		count--;
// Still locked
		if(count > 0) 
		{
			pthread_mutex_unlock(&recursive_lock);
			return 0;
		}
// Not owned anymore
		thread_id = 0;
		thread_id_valid = 0;
		pthread_mutex_unlock(&recursive_lock);
	}
	else
		count = 0;


#ifndef NO_GUICAST
	UNSET_LOCK(this);
#endif

	if(pthread_mutex_unlock(&mutex)) perror("Mutex::unlock");
	return 0;
}

int Mutex::trylock()
{
	return pthread_mutex_trylock(&mutex);
}

int Mutex::is_locked()
{
	return count;
}

int Mutex::reset()
{
	pthread_mutex_destroy(&mutex);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&mutex, &attr);
	count = 0;
	thread_id = 0;
	thread_id_valid = 0;
#ifndef NO_GUICAST
	UNSET_ALL_LOCKS(this)
#endif
	return 0;
}
