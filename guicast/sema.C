
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

