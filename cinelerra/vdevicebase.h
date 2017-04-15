
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

#ifndef VDEVICEBASE_H
#define VDEVICEBASE_H

#include "asset.inc"
#include "assets.inc"
#include "bcbitmap.inc"
#include "edl.inc"
#include "videodevice.inc"

class VDeviceBase
{
public:
	VDeviceBase(VideoDevice *device);
	virtual ~VDeviceBase() {};

	virtual int write_buffer(VFrame *output, EDL *edl) { return 1; };
	virtual void new_output_buffer(VFrame **output, int colormodel) {};
// Extra work must sometimes be done in here to set up the device.

	virtual int open_output() { return 1; };
	virtual int output_visible() { return 0; };
	virtual BC_Bitmap* get_bitmap() { return 0; };

	VideoDevice *device;
};

#endif
