#include "drivesync.h"

#include <stdio.h>
#include <unistd.h>

DriveSync::DriveSync()
 : Thread()
{
	done = 0;
	set_synchronous(1);
}

DriveSync::~DriveSync()
{
	done = 1;
	Thread::join();
}

void DriveSync::run()
{
// Task switching too slow on alpha
	while(!done)
	{
		sync();
		sleep(1);
	}
}
