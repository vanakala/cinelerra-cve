#ifndef DRIVESYNC_H
#define DRIVESYNC_H

#include "thread.h"

class DriveSync : public Thread
{
public:
	DriveSync();
	~DriveSync();

	void run();

	int done;
};

#endif
