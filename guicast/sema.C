#include "bcsignals.h"
#include "sema.h"




Sema::Sema(int init_value, char *title)
{
	sem_init(&sem, 0, init_value);
	this->title = title;
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
