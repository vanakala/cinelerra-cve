#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <stdio.h>

class Mutex
{
public:
	Mutex();
	~Mutex();

	int lock();
	int unlock();
	int trylock();
	int reset();

	pthread_mutex_t mutex;
};


#endif
