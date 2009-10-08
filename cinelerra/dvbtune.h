
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef DVBTUNE_H
#define DVBTUNE_H




/**
 * Interface for digital tuners using DVB.
 **/

#include "condition.inc"
#include "dvbtune.inc"
#include "mutex.inc"
#include "renderfarmclient.inc"
#include "thread.h"
#include "tunerserver.h"






class DVBTune : public TunerServer
{
public:
	DVBTune(RenderFarmClientThread *client);
	~DVBTune();
	
	void reset();

	int open_tuner();
	int close_tuner();
	int get_signal_strength(int *current_power, int *current_lock);
	int read_data(unsigned char *data, int size);




	int frontend_fd;
	int audio_fd;
	int video_fd;
	int dvr_fd;

	Mutex *buffer_lock;
	unsigned char *buffer;
	int buffer_size;
	int buffer_allocated;
	DVBTuneThread *thread;
	DVBTuneStatus *status;
	int has_lock;
};

// Read asynchronously in case the network was too slow.
// Turns out the status was the thing slowing it down.

class DVBTuneThread : public Thread
{
public:
	DVBTuneThread(DVBTune *server);
	~DVBTuneThread();
	void run();
	DVBTune *server;
	unsigned char *temp;
};

// Need to get tuner status separately because it's real slow.

class DVBTuneStatus : public Thread
{
public:
	DVBTuneStatus(DVBTune *server);
	~DVBTuneStatus();
	void run();
	DVBTune *server;
};









#endif
