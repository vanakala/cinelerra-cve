#ifndef BCREPEATER_H
#define BCREPEATER_H

#include "bcrepeater.inc"
#include "bcwindowbase.inc"
#include "condition.inc"
#include "thread.h"
#include "bctimer.h"

class BC_Repeater : public Thread
{
public:
	BC_Repeater(BC_WindowBase *top_level, long delay);
	~BC_Repeater();

	void initialize();
	int start_repeating();
	int stop_repeating();
	void run();

	long repeat_id;
	long delay;
	int repeating;
	int interrupted;
// Prevent new signal until existing event is processed
	Condition *repeat_lock;

private:
	Timer timer;
	BC_WindowBase *top_level;
// Delay corrected for the time the last repeat took
	long next_delay;
	Condition *pause_lock;
	Condition *startup_lock;
};



#endif
