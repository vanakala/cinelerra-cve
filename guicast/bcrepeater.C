#include "bcrepeater.h"
#include "bcwindow.h"
#include "condition.h"

#include <unistd.h>

BC_Repeater::BC_Repeater(BC_WindowBase *top_level, long delay)
 : Thread()
{
	set_synchronous(1);
	pause_lock = new Condition(0, "BC_Repeater::pause_lock");
	startup_lock = new Condition(0, "BC_Repeater::startup_lock");
	repeat_lock = new Condition(1, "BC_Repeater::repeat_lock");
	repeating = 0;
	interrupted = 0;
	this->delay = delay;
	this->top_level = top_level;

}

BC_Repeater::~BC_Repeater()
{
	interrupted = 1;
	pause_lock->unlock();
	repeat_lock->unlock();

	Thread::end();
	Thread::join();

	delete pause_lock;
	delete startup_lock;
	delete repeat_lock;
}

void BC_Repeater::initialize()
{
	start();
// This is important.  It doesn't unblock until the thread is running.
	startup_lock->lock("BC_Repeater::initialize");
}

int BC_Repeater::start_repeating()
{
//printf("BC_Repeater::start_repeating 1 %d\n", delay);
	repeating++;
	if(repeating == 1)
	{
// Resume the loop
		pause_lock->unlock();
	}
	return 0;
}

int BC_Repeater::stop_repeating()
{
// Recursive calling happens when mouse wheel is used.
	if(repeating > 0)
	{
		repeating--;
// Pause the loop
		if(repeating == 0) pause_lock->lock("BC_Repeater::stop_repeating");
	}
	return 0;
}

void BC_Repeater::run()
{
//printf("BC_Repeater::run 1 %d\n", getpid());
	next_delay = delay;
	Thread::disable_cancel();
	startup_lock->unlock();

	while(!interrupted)
	{
		Thread::enable_cancel();
		timer.delay(next_delay);
		Thread::disable_cancel();
//if(next_delay <= 0) printf("BC_Repeater::run delay=%d next_delay=%d\n", delay, next_delay);

// Test exit conditions
		if(interrupted) return;
// Busy wait here
//		if(repeating <= 0) continue;

// Test for pause
		pause_lock->lock("BC_Repeater::run");
		pause_lock->unlock();
		timer.update();

// Test exit conditions
		if(interrupted) return;
		if(repeating <= 0) continue;

// Wait for existing signal to be processed before sending a new one
		repeat_lock->lock("BC_Repeater::run");

// Test exit conditions
		if(interrupted)
		{
			repeat_lock->unlock();
			return;
		}
		if(repeating <= 0)
		{
			repeat_lock->unlock();
			continue;
		}

// Wait for window to become available.
		top_level->lock_window("BC_Repeater::run");

// Test exit conditions
		if(interrupted)
		{
			repeat_lock->unlock();
			top_level->unlock_window();
			return;
		}
		if(repeating <= 0)
		{
			repeat_lock->unlock();
			top_level->unlock_window();
			continue;
		}

// Stick event into queue
		top_level->arm_repeat(delay);
		top_level->unlock_window();
		next_delay = delay - timer.get_difference();
		if(next_delay <= 0) next_delay = 0;

// Test exit conditions
		if(interrupted) 
		{
			repeat_lock->unlock();
			return;
		}
		if(repeating <= 0)
		{
			repeat_lock->unlock();
			continue;
		}
	}
}
