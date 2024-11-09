// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "mutex.h"

Mutex::Mutex(const char *title, int recursive)
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
	UNSET_ALL_LOCKS(this);
}

void Mutex::lock(const char *location)
{
	int rv;

// Test recursive owner and give up if we already own it
	if(recursive)
	{
		pthread_mutex_lock(&recursive_lock);
		if(thread_id_valid && pthread_self() == thread_id) 
		{
			count++;
			pthread_mutex_unlock(&recursive_lock);
			return;
		}
		pthread_mutex_unlock(&recursive_lock);
	}

	SET_MLOCK(this, title, location);

	if(rv = pthread_mutex_lock(&mutex))
		fprintf(stderr, "%s:%s mutex lock failed with %d\n", title, location, rv);

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
		count = 1;

	SET_LOCK2
}

void Mutex::unlock()
{
	int rv;

// Remove from recursive status
	if(recursive)
	{
		pthread_mutex_lock(&recursive_lock);
		count--;
// Still locked
		if(count > 0) 
		{
			pthread_mutex_unlock(&recursive_lock);
			return;
		}
// Not owned anymore
		thread_id = 0;
		thread_id_valid = 0;
		pthread_mutex_unlock(&recursive_lock);
	}
	else
		count = 0;

	UNSET_LOCK(this);

	if(rv = pthread_mutex_unlock(&mutex))
		fprintf(stderr, "%s mutex unlock failed with %d\n", title, rv);
}

int Mutex::trylock()
{
	return pthread_mutex_trylock(&mutex);
}

int Mutex::is_locked()
{
	return count;
}

void Mutex::reset()
{
	pthread_mutex_destroy(&mutex);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&mutex, &attr);
	count = 0;
	thread_id = 0;
	thread_id_valid = 0;
	UNSET_ALL_LOCKS(this)
}
