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
//printf("DriveSync::run 1\n");
		sync();
//printf("DriveSync::run 10\n");
		sleep(1);
	}
}
