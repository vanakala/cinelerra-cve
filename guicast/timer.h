#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>


class Timer
{
public:
	Timer();
	virtual ~Timer();
	
	int update();                // set last update to now
	
// get difference between now and last update in milliseconds
// must be positive or error results
	long long get_difference(struct timeval *result); // also stores in timeval
	long long get_difference();

// get difference in arbitrary units between now and last update    
	long long get_scaled_difference(long denominator);        
	int delay(long milliseconds);

private:
	struct timeval current_time;
	struct timeval new_time;
	struct timeval delay_duration;
};



#endif
