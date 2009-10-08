
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

#ifndef TUNERSERVER_H
#define TUNERSERVER_H



#include "renderfarmclient.inc"
#include "thread.h"
#include "tunerserver.inc"

#include <stdint.h>



/**
 * Inherited by all network tuners.
 **/

class TunerServer
{
public:
	TunerServer(RenderFarmClientThread *client);
	virtual ~TunerServer();


/**
 * User calls this to initialize and run the server
 **/
	void main_loop();

/**
 * These are overridden.  Returns of 1 are considered errors.
 **/
	virtual int open_tuner();
	virtual int close_tuner();
	virtual int get_signal_strength(int *current_power, int *current_lock);
/**
 * Should return the number of bytes read.
 **/
	virtual int read_data(unsigned char *data, int size);


	int get_table();
	int get_channel();
	int get_audio_pid();
	int get_video_pid();
	int get_device_number();

private:

	int is_busy;
	RenderFarmClientThread *client;
	int port;
// Channel number
	int channel;
// Channel table
	int table;
	int audio_pid;
	int video_pid;
	int device_number;
	unsigned char *temp;
	int temp_allocated;
	int connection_closed;
};




#endif
