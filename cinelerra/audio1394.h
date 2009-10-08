
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

#ifndef AUDIO1394_H
#define AUDIO1394_H

#include "audiodevice.h"
#include "device1394input.inc"
#include "device1394output.inc"
#include "iec61883input.inc"
#include "iec61883output.inc"
#include "vdevice1394.inc"



#include "libdv.h"

class Audio1394 : public AudioLowLevel
{
public:
	Audio1394(AudioDevice *device);
	~Audio1394();


	friend class VDevice1394;

	int initialize();

	int open_input();
	int open_output();
	int close_all();
	int read_buffer(char *buffer, int bytes);
	int write_buffer(char *buffer, int bytes);
	int64_t device_position();
	int flush_device();
	int interrupt_playback();

	
private:
	Device1394Input *input_thread;
	Device1394Output *output_thread;
	IEC61883Input *input_iec;
	IEC61883Output *output_iec;
	int bytes_per_sample;
};

#endif
