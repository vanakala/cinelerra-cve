#ifndef CONDITION_H
#define CONDITION_H

#include <pthread.h>

class Condition
{
public:
	Condition(int init_value = 0, char *title = 0);
	~Condition();

// Reset to init_value whether locked or not.
	void reset();
// Block if value <= 0, then decrease value
	void lock(char *location = 0);
// Increase value
	void unlock();
// Block for requested duration if value <= 0.
// value is decreased whether or not the condition is unlocked in time
	int timed_lock(int microseconds, char *location = 0);
	int get_value();

    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int value;
	int init_value;
	char *title;
};


#endif
