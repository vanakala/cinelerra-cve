
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

#ifndef VDEVICE1394_H
#define VDEVICE1394_H

#include "device1394input.inc"
#include "device1394output.inc"
#include "guicast.h"
#include "iec61883input.inc"
#include "iec61883output.inc"
#include "libdv.h"
#include "quicktime.h"
#include "sema.h"
#include "vdevicebase.h"





class VDevice1394 : public VDeviceBase
{
public:
	VDevice1394(VideoDevice *device);
	~VDevice1394();

	int open_input();
	int open_output();
	int close_all();
	int read_buffer(VFrame *frame);
	int write_buffer(VFrame *frame, EDL *edl);
// Called by the audio device to share a buffer
//	int get_shared_data(unsigned char *data, long size);
	int initialize();
	int can_copy_from(Asset *asset, int output_w, int output_h);
//	int stop_sharing();
	void new_output_buffer(VFrame **output, int colormodel);
	void encrypt(unsigned char *output, unsigned char *data, int data_size);

private:
	Device1394Input *input_thread;
	Device1394Output *output_thread;
	IEC61883Input *input_iec;
	IEC61883Output *output_iec;
	VFrame *user_frame;
};

#endif
