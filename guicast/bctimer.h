#ifndef BCTIMER_H
#define BCTIMER_H

#include <stdint.h>
#include <sys/time.h>


class Timer
{
public:
	Timer();
	virtual ~Timer();
	
	int update();                // set last update to now
	
// get difference between now and last update in milliseconds
// must be positive or error results
	int64_t get_difference(struct timeval *result); // also stores in timeval
	int64_t get_difference();

// get difference in arbitrary units between now and last update    
	int64_t get_scaled_difference(long denominator);        
	int delay(long milliseconds);

private:
	struct timeval current_time;
	struct timeval new_time;
	struct timeval delay_duration;
};



#endif
