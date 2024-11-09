// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CONDITION_H
#define CONDITION_H

#include <pthread.h>

class Condition
{
public:
	Condition(int init_value = 0, const char *title = 0, int is_binary = 0);
	~Condition();

// Reset to init_value whether locked or not.
	void reset();
// Block if value <= 0, then decrease value
	void lock(const char *location = 0);
// Increase value
	void unlock();
// Block if value > 0, else release lock
// Usage: wait slower thread
	void wait_another(const char *location = 0);
// Block for requested duration if value <= 0.
// value is decreased whether or not the condition is unlocked in time
// Returns 1 if timeout or 0 if success.
	int timed_lock(int microseconds, const char *location = 0);
	int get_value();

	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int value;
	int init_value;
	int is_binary;
	const char *title;
};

#endif
