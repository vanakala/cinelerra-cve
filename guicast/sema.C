// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "sema.h"

Sema::Sema(int init_value, const char *title)
{
	sem_init(&sem, 0, init_value);
	this->title = title;
	this->init_value = init_value;
}

Sema::~Sema()
{
	sem_destroy(&sem);
	UNSET_ALL_LOCKS(this);
}

void Sema::lock(const char *location)
{
	SET_SLOCK(this, title, location);
	sem_wait(&sem);
	SET_LOCK2
}

void Sema::unlock()
{
	UNSET_LOCK(this);
	sem_post(&sem);
}

int Sema::get_value()
{
	int result;
	sem_getvalue(&sem, &result);
	return result;
}

void Sema::reset()
{
	sem_destroy(&sem);
	sem_init(&sem, 0, init_value);
	UNSET_ALL_LOCKS(this)
}
