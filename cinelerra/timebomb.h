#ifndef TIMEBOMB_H
#define TIMEBOMB_H

#include <time.h>

class TimeBomb
{
public:
	TimeBomb();
	~TimeBomb();

	int test_time(time_t testtime);
	void disable_system();
};




#endif
