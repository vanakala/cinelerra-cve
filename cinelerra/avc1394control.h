#ifndef _AVC1394Control_H
#define _AVC1394Control_H

#include "mutex.h"

#include <libavc1394/rom1394.h>
#include <libraw1394/raw1394.h>
#include <libavc1394/avc1394.h>
#include <libavc1394/avc1394_vcr.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

class AVC1394Control
{
public:
	AVC1394Control();
	~AVC1394Control();

	void initialize();

	void play();
	void reverse();
	void stop();
	void pause();
	void rewind();
	void fforward();
	void record();
	void eject();
	void get_status();
	void seek(char *time);
	char *timecode();
	int device;
	int status;
	Mutex *device_lock;

private:
	rom1394_directory rom_dir;
	raw1394handle_t handle;
	
};

#endif
