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

