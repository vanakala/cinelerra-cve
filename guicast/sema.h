
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

#ifndef SEMA_H
#define SEMA_H


#include <semaphore.h>

class Sema
{
public:
	Sema(int init_value = 1, char *title = 0);
	~Sema();

	void lock(char *location = 0);
	void unlock();
	int get_value();
	void reset();

	sem_t sem;
	char *title;
	int init_value;
};








#if 0


#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

union semun {
   int val; /* used for SETVAL only */
   struct semid_ds *buf; /* for IPC_STAT and IPC_SET */
   ushort *array;  /* used for GETALL and SETALL */
};

class Sema
{
public:
	Sema(int id = -1, int number = 1);
	~Sema();

	int lock(int number = 0);
	int unlock(int number = 0);
	int get_value(int number = 0);
	int get_id();

	int semid;
	int client;
	int semas;
	semun arg;
};

#endif





#endif
