#ifndef SHMEM_H
#define SHMEM_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

class SharedMem
{
public:
	SharedMem(long size);
	SharedMem(int id, long size);
	~SharedMem();

	int get_id();
	long get_size();

	char *data;
	long size;
	int shmid;
	int client;
};


#endif
