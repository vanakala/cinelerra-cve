#ifndef BCREPEATER_H
#define BCREPEATER_H

#include "bcrepeater.inc"
#include "bcwindowbase.inc"
#include "mutex.h"
#include "thread.h"
#include "timer.h"

class BC_Repeater : public Thread
{
public:
	BC_Repeater(BC_WindowBase *top_level, long delay);
	~BC_Repeater();

	int start_repeating();
	int stop_repeating();
	void run();

	long repeat_id;
	long delay;
	int repeating;
	int interrupted;
// Prevent new signal until existing event is processed
	Mutex repeat_lock;

private:
	Timer timer;
	BC_WindowBase *top_level;
// Delay corrected for the time the last repeat took
	long next_delay;
	Mutex pause_lock;
	Mutex startup_lock;
};



#endif
