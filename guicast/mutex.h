#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <stdio.h>

class Mutex
{
public:
	Mutex(char *title = 0);
	~Mutex();

	int lock(char *location = 0);
	int unlock();
	int trylock();
	int reset();

	pthread_mutex_t mutex;
	char *title;
};


#endif
