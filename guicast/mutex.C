
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef NO_GUICAST
#include "bcsignals.h"
#endif
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
#ifndef NO_GUICAST
	UNSET_ALL_LOCKS(this);
#endif
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

#ifndef NO_GUICAST
	SET_MLOCK(this, title, location);
#endif
	if(rv = pthread_mutex_lock(&mutex))
		fprintf(stderr, "%s mutex lock failed with %d", title, rv);

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

#ifndef NO_GUICAST
	UNSET_LOCK(this);
#endif

	if(rv = pthread_mutex_unlock(&mutex))
		fprintf(stderr, "%s mutex unlock failed with %d", title, rv);
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
#ifndef NO_GUICAST
	UNSET_ALL_LOCKS(this)
#endif
}
