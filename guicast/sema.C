
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




Sema::Sema(int init_value, char *title)
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


void Sema::lock(char *location)
{
	SET_LOCK(this, title, location);
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



















#if 0

#include "bcipc.h"
#include "sema.h"

Sema::Sema(int id, int number)
{
	if(id == -1)
	{
		if((semid = semget(IPC_PRIVATE, number, IPC_CREAT | 0777)) < 0) perror("Sema::Sema");
		for(int i = 0; i < number; i++) unlock(i);
		client = 0;
		bc_enter_sema_id(semid);
	}
	else
	{
		client = 1;
		this->semid = id;
	}

	semas = number;
}

Sema::~Sema()
{
	if(!client)
	{
		if(semctl(semid, 0, IPC_RMID, arg) < 0) perror("Sema::~Sema");
		bc_remove_sema_id(semid);
	}
}

	
int Sema::lock(int number)
{
	struct sembuf sop;
	
// decrease the semaphore
	sop.sem_num = number;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	if(semop(semid, &sop, 1) < 0) perror("Sema::lock");
	return 0;
}

int Sema::unlock(int number)
{
	struct sembuf sop;
	
// decrease the semaphore
	sop.sem_num = number;
	sop.sem_op = 1;
	sop.sem_flg = 0;
	if(semop(semid, &sop, 1) < 0) perror("Sema::unlock");
	return 0;
}

int Sema::get_value(int number)
{
	return semctl(semid, number, GETVAL, arg);
}

int Sema::get_id()
{
	return semid;
}


#endif
