
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

#include "asset.h"
#include "devicedvbinput.inc"
#include "file.inc"
#include "mwindow.h"
#include "vdevicedvb.h"
#include "videodevice.h"
#include "devicedvbinput.h"


#include <unistd.h>

VDeviceDVB::VDeviceDVB(VideoDevice *device)
 : VDeviceBase(device)
{
	initialize();
}

VDeviceDVB::~VDeviceDVB()
{
	close_all();
}


int VDeviceDVB::initialize()
{
	input_thread = 0;
	return 0;
}

int VDeviceDVB::open_input()
{
	if(!input_thread)
	{
		input_thread = DeviceDVBInput::get_input_thread(device->mwindow);
	}
sleep(100);

	if(input_thread)
		return 0;
	else
		return 0;
}

int VDeviceDVB::close_all()
{
	if(input_thread)
	{
		DeviceDVBInput::put_input_thread(device->mwindow);
	}
	input_thread = 0;
	return 0;
}

int VDeviceDVB::read_buffer(VFrame *frame)
{
	return 0;
}

void VDeviceDVB::fix_asset(Asset *asset)
{
	asset->format = FILE_MPEG;
// This file writer should ignore codec fields.
}




