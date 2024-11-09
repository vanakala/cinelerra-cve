// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "condition.h"

#include <errno.h>
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
	UNSET_ALL_LOCKS(this);
}

void Condition::reset()
{
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	UNSET_ALL_LOCKS(this);
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, NULL);
	value = init_value;
}

void Condition::lock(const char *location)
{
	pthread_mutex_lock(&mutex);
	SET_CLOCK(this, title, location);

	while(value <= 0)
		pthread_cond_wait(&cond, &mutex);

	UNSET_LOCK2

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

void Condition::wait_another(const char *location)
{
	pthread_mutex_lock(&mutex);
	if(value)
		value = 0;
	else
	{
		value = 1;
		pthread_cond_signal(&cond);
	}
	SET_CLOCK(this, title, location);

	while(value <= 0)
		pthread_cond_wait(&cond, &mutex);

	UNSET_LOCK2
	pthread_mutex_unlock(&mutex);
}

int Condition::timed_lock(int microseconds, const char *location)
{
	struct timeval now;
	struct timespec timeout;
	int result = 0;

	pthread_mutex_lock(&mutex);
	SET_CLOCK(this, title, location);
	gettimeofday(&now, 0);
	timeout.tv_sec = now.tv_sec + microseconds / 1000000;
	timeout.tv_nsec = now.tv_usec * 1000 + (microseconds % 1000000) * 1000;

	while(value <= 0 && result != ETIMEDOUT)
		result = pthread_cond_timedwait(&cond, &mutex, &timeout);

	UNSET_LOCK2

	if(result == ETIMEDOUT)
		result = 1;
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
