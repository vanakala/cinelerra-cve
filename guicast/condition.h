#ifndef CONDITION_H
#define CONDITION_H

#include <pthread.h>

class Condition
{
public:
	Condition(int init_value = 0);
	~Condition();

// Block if value <= 0, then decrease value
	void lock();
// Increase value
	void unlock();
	int timed_lock(int milliseconds);
	
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int value;
};


#endif
