
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
#include "condition.h"

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

Condition::Condition(int init_value, const char *title, int is_binary)
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
#ifndef NO_GUICAST
	UNSET_ALL_LOCKS(this);
#endif
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, NULL);
	value = init_value;
}

void Condition::lock(const char *location)
{
	pthread_mutex_lock(&mutex);
#ifndef NO_GUICAST
	SET_CLOCK(this, title, location);
#endif
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
	pthread_mutex_lock(&mutex);
// The lock trace is created and removed by the acquirer
	if(is_binary)
		value = 1;
	else
		value++;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

int Condition::timed_lock(int microseconds, const char *location)
{
	struct timeval now;
	struct timespec timeout;
	int result = 0;

	pthread_mutex_lock(&mutex);
#ifndef NO_GUICAST
	SET_CLOCK(this, title, location);
#endif
	gettimeofday(&now, 0);
	timeout.tv_sec = now.tv_sec + microseconds / 1000000;
	timeout.tv_nsec = now.tv_usec * 1000 + (microseconds % 1000000) * 1000;

	while(value <= 0 && result != ETIMEDOUT)
	{
		result = pthread_cond_timedwait(&cond, &mutex, &timeout);
	}
#ifndef NO_GUICAST
		UNSET_LOCK2
#endif

	if(result == ETIMEDOUT) 
	{
		result = 1;
	}
	else
	{
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
