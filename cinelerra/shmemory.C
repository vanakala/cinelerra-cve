
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

#include <string.h>
#include "bcipc.h"
#include "shmemory.h"


SharedMem::SharedMem(long size)
{
	shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
	if(shmid < 0)
		perror("SharedMem::SharedMem");
	else
	{
		data = (char*)shmat(shmid, 0, 0);
		shmctl(shmid, IPC_RMID, 0);
	}
	this->size = size;
	client = 0;
}

SharedMem::SharedMem(int id, long size)
{
	this->shmid = id;

	data = (char*)shmat(shmid, 0, 0);
	this->size = size;
	client = 1;
}

SharedMem::~SharedMem()
{
	shmdt(data);
	data = 0;
	size = 0;
	shmid = 0;
}

int SharedMem::get_id()
{
	return shmid;
}

long SharedMem::get_size()
{
	return size;
}

