#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <stdio.h>

class Mutex
{
public:
	Mutex(char *title = 0, int recursive = 0);
	~Mutex();

	int lock(char *location = 0);
	int unlock();
// Calls pthread_mutex_trylock, whose effect depends on library version.
	int trylock();
	int reset();
// Returns 1 if count is > 0
	int is_locked();

// Number of times the thread currently holding the mutex has locked it.
// For recursive locking.
	int count;
// ID of the thread currently holding the mutex.  For recursive locking.
	pthread_t thread_id;
	int thread_id_valid;
	int recursive;
// Lock the variables for recursive locking.
	pthread_mutex_t recursive_lock;
	pthread_mutex_t mutex;
	char *title;
};


#endif
