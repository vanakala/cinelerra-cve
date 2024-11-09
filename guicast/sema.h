// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SEMA_H
#define SEMA_H

#include <semaphore.h>

class Sema
{
public:
	Sema(int init_value = 1, const char *title = 0);
	~Sema();

	void lock(const char *location = 0);
	void unlock();
	int get_value();
	void reset();

	sem_t sem;
	const char *title;
	int init_value;
};

#endif
