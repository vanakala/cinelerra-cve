
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

// V4L2 is incompatible with large file support
#undef _FILE_OFFSET_BITS
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE


#include "assets.h"
#include "channel.h"
#include "chantables.h"
#include "clip.h"
#include "condition.h"
#include "file.h"
#include "libmjpeg.h"
#include "picture.h"
#include "preferences.h"
#include "quicktime.h"
#include "recordconfig.h"
#include "vdevicev4l2jpeg.h"
#include "vdevicev4l2.h"
#include "vframe.h"
#include "videodevice.h"

#ifdef HAVE_VIDEO4LINUX2

#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>








VDeviceV4L2JPEG::VDeviceV4L2JPEG(VideoDevice *device)
 : VDeviceBase(device)
{
	initialize();
}

VDeviceV4L2JPEG::~VDeviceV4L2JPEG()
{
	close_all();
}

int VDeviceV4L2JPEG::initialize()
{
	thread = 0;
}


int VDeviceV4L2JPEG::open_input()
{
	return VDeviceV4L2::get_sources(device,
		device->in_config->v4l2jpeg_in_device);
}

int VDeviceV4L2JPEG::close_all()
{
	if(thread) delete thread;
	thread = 0;
	return 0;
}

int VDeviceV4L2JPEG::get_best_colormodel(Asset *asset)
{
	return BC_COMPRESSED;
}

int VDeviceV4L2JPEG::read_buffer(VFrame *frame)
{
	int result = 0;

	if((device->channel_changed || device->picture_changed) && thread)
	{
		delete thread;
		thread = 0;
	}

	if(!thread)
	{
		device->channel_changed = 0;
		device->picture_changed = 0;
		thread = new VDeviceV4L2Thread(device, BC_COMPRESSED);
		thread->start();
	}


// Get buffer from thread
	int timed_out;
	VFrame *buffer = thread->get_buffer(&timed_out);



	if(buffer)
	{
		frame->allocate_compressed_data(buffer->get_compressed_size());
		frame->set_compressed_size(buffer->get_compressed_size());

// Transfer fields to frame
		if(device->odd_field_first)
		{
			int field2_offset = mjpeg_get_field2((unsigned char*)buffer->get_data(), 
				buffer->get_compressed_size());
			int field1_len = field2_offset;
			int field2_len = buffer->get_compressed_size() - 
				field2_offset;

			memcpy(frame->get_data(), 
				buffer->get_data() + field2_offset, 
				field2_len);
			memcpy(frame->get_data() + field2_len, 
				buffer->get_data(), 
				field1_len);
		}
		else
		{
			bcopy(buffer->get_data(), 
				frame->get_data(), 
				buffer->get_compressed_size());
		}

		thread->put_buffer();
	}
	else
	{
// Driver in 2.6.3 needs to be restarted when it loses sync.
		if(timed_out)
		{
			delete thread;
			thread = 0;
		}
		result = 1;
	}


	return result;
}



#endif


