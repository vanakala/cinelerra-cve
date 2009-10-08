
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

#ifndef AUDIODVB_H
#define AUDIODVB_H


#include "audiodevice.h"
#include "devicedvbinput.inc"
#include "vdevicedvb.inc"



// This reads audio from the DVB input and output uncompressed audio only.
// Used for the LiveAudio plugin and previewing.




class AudioDVB : public AudioLowLevel
{
public:
	AudioDVB(AudioDevice *device);
	~AudioDVB();
	
	
	friend class VDeviceDVB;
	
	int open_input();
	int close_all();
	void reset();


	DeviceDVBInput *input_thread;
};







#endif
