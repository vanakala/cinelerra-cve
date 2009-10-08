
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

#include "dv1394input.h"



DV1394Input::DV1394Input(VideoDevice *device)
 : Thread(1, 1, 0)
{
	this->device = device;
	done = 0;
	fd = 0;
}


DV1394Input::~DV1394Input()
{
	if(Thread::running())
	{
		done = 1;
		Thread::cancel();
		Thread::join();
	}

	if(fd > 0) close(fd);
}

void DV1394Input::start()
{
	Thread::start();
}



void DV1394Input::run()
{
// Open the device
	int error = 0;
	if((fd = open(device->in_config->dv1394_path, O_RDWR)) < 0)
	{
		fprintf(stderr, 
			"DV1394Input::run path=s: %s\n", 
			device->in_config->dv1394_path, 
			strerror(errno));
		error = 1;
	}

	if(!error)
	{
    	struct dv1394_init init = 
		{
    	   api_version: DV1394_API_VERSION,
    	   channel:     device->out_config->dv1394_channel,
  		   n_frames:    length,
    	   format:      is_pal ? DV1394_PAL : DV1394_NTSC,
    	   cip_n:       0,
    	   cip_d:       0,
    	   syt_offset:  syt
    	};

		
	}

	while(!done)
	{
		
	}
}
