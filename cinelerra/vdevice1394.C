
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

#include "assets.h"
#include "audio1394.h"
#include "audioconfig.h"
#include "audiodevice.h"
#include "device1394input.h"
#include "device1394output.h"
#include "iec61883input.h"
#include "iec61883output.h"
#include "file.inc"
#include "preferences.h"
#include "recordconfig.h"
#include "vdevice1394.h"
#include "vframe.h"
#include "playbackconfig.h"
#include "videodevice.h"










VDevice1394::VDevice1394(VideoDevice *device)
 : VDeviceBase(device)
{
	initialize();
}

VDevice1394::~VDevice1394()
{
	close_all();
}

int VDevice1394::initialize()
{
	input_thread = 0;
	output_thread = 0;
	input_iec = 0;
	output_iec = 0;
	user_frame = 0;
}

int VDevice1394::open_input()
{
// Share audio driver.  The audio driver does the capturing in this case
// and fills video frames for us.
	int result = 0;
	if(device->adevice &&
		(device->adevice->in_config->driver == AUDIO_1394 ||
			device->adevice->in_config->driver == AUDIO_DV1394 ||
			device->adevice->in_config->driver == AUDIO_IEC61883))
	{
		Audio1394 *low_level = (Audio1394*)device->adevice->get_lowlevel_in();
		input_thread = low_level->input_thread;
		input_iec = low_level->input_iec;
		device->sharing = 1;
	}
	else
	if(!input_thread && !input_iec)
	{
		if(device->in_config->driver == CAPTURE_FIREWIRE)
		{
			input_thread = new Device1394Input;
			result = input_thread->open(device->in_config->firewire_path,
				device->in_config->firewire_port, 
				device->in_config->firewire_channel, 
				device->in_config->capture_length,
				2,
				48000,
				16,
				device->in_config->w,
				device->in_config->h);
			if(result)
			{
				delete input_thread;
				input_thread = 0;
			}
		}
		else
		{
			input_iec = new IEC61883Input;
			result = input_iec->open(device->in_config->firewire_port, 
				device->in_config->firewire_channel, 
				device->in_config->capture_length,
				2,
				48000,
				16,
				device->in_config->w,
				device->in_config->h);
			if(result)
			{
				delete input_iec;
				input_iec = 0;
			}
		}
	}
	return result;
}

int VDevice1394::open_output()
{
// Share audio driver.  The audio driver takes DV frames from us and
// inserts audio.
	if(device->adevice &&
		(device->adevice->out_config->driver == AUDIO_1394 ||
			device->adevice->out_config->driver == AUDIO_DV1394 ||
			device->adevice->out_config->driver == AUDIO_IEC61883))
	{
		Audio1394 *low_level = (Audio1394*)device->adevice->get_lowlevel_out();
		output_thread = low_level->output_thread;
		output_iec = low_level->output_iec;
		device->sharing = 1;
	}
	else
	if(!output_thread && !output_iec)
	{
		if(device->out_config->driver == PLAYBACK_DV1394)
		{
			output_thread = new Device1394Output(device);
			output_thread->open(device->out_config->dv1394_path,
				device->out_config->dv1394_port, 
				device->out_config->dv1394_channel,
				30,
				2,
				16,
				48000,
				device->out_config->dv1394_syt);
		}
		else
		if(device->out_config->driver == PLAYBACK_FIREWIRE)
		{
			output_thread = new Device1394Output(device);
			output_thread->open(device->out_config->firewire_path,
				device->out_config->firewire_port, 
				device->out_config->firewire_channel,
				30,
				2,
				16,
				48000,
				device->out_config->firewire_syt);
		}
		else
		{
			output_iec = new IEC61883Output(device);
			output_iec->open(device->out_config->firewire_port, 
				device->out_config->firewire_channel,
				30,
				2,
				16,
				48000,
				device->out_config->firewire_syt);
		}
	}

	return 0;
}

int VDevice1394::close_all()
{
	if(device->sharing)
	{
		input_thread = 0;
		output_thread = 0;
		output_iec = 0;
		input_iec = 0;
		device->sharing = 0;
	}
	else
	{
		if(input_thread)
		{
			delete input_thread;
			input_thread = 0;
		}
		if(output_thread)
		{
			delete output_thread;
			output_thread = 0;
		}
		if(input_iec)
		{
			delete input_iec;
			input_iec = 0;
		}
		if(output_iec)
		{
			delete output_iec;
			output_iec = 0;
		}
	}
	if(user_frame) delete user_frame;
	initialize();
	return 0;
}


int VDevice1394::read_buffer(VFrame *frame)
{
	unsigned char *data;
	long size = 0;
	int result = 0;
	if(!input_thread && !input_iec) return 1;

	if(input_thread) input_thread->read_video(frame);
	else
	if(input_iec) input_iec->read_video(frame);

	return result;
}


void VDevice1394::new_output_buffer(VFrame **output,
	int colormodel)
{
	if(user_frame)
	{
		if(colormodel != user_frame->get_color_model())
		{
			delete user_frame;
			user_frame = 0;
		}
	}

	if(!user_frame)
	{
		switch(colormodel)
		{
			case BC_COMPRESSED:
				user_frame = new VFrame;
				break;
			default:
				user_frame = new VFrame(0,
					device->out_w,
					device->out_h,
					colormodel,
					-1);
				break;
		}
	}
	user_frame->set_shm_offset(0);
	*output = user_frame;
}

int VDevice1394::write_buffer(VFrame *frame, EDL *edl)
{
	if(output_thread) output_thread->write_frame(frame);
	else
	if(output_iec) output_iec->write_frame(frame);
	return 0;
}




int VDevice1394::can_copy_from(Asset *asset, int output_w, int output_h)
{
	return 0;
}
