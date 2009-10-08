
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

// Based off of the dvcont test app included with libavc1394.
// (Originally written by Jason Howard [And based off of gscanbus],
// adapted to libavc1394 by Dan Dennedy <dan@dennedy.org>. Released under
// the GPL.)

#include "avc1394control.h"
#include "mutex.h"
#include "transportque.inc"

AVC1394Control::AVC1394Control()
{
	initialize();
}

void AVC1394Control::initialize()
{
	int i;

	current_command = COMMAND_NONE;
	device = -1;

	device_lock = new Mutex("AVC1394Control::device_lock");

#ifdef RAW1394_V_0_8
	handle = raw1394_get_handle();
#else
	handle = raw1394_new_handle();
#endif
//printf("AVC1394Control::initialize(): 1\n");
	if(!handle)
	{
//printf("AVC1394Control::initialize(): 2\n");
		if(!errno)
		{
//printf("AVC1394Control::initialize(): 3\n");
			fprintf(stderr, "AVC1394Control::initialize(): Not Compatable!\n");
		} 
		else 
		{
//printf("AVC1394Control::initialize(): 4\n");
			fprintf(stderr, "AVC1394Control::initialize(): couldn't get handle\n");
		}
		return;
	}

	if(raw1394_set_port(handle, 0) < 0) {
//printf("AVC1394Control::initialize(): 5\n");
		perror("AVC1394Control::initialize(): couldn't set port");
//		raw1394_destroy_handle(handle);
		return;
	}

	for(i = 0; i < raw1394_get_nodecount(handle); i++)
	{
		if(rom1394_get_directory(handle, i, &rom_dir) < 0)
		{
//printf("AVC1394Control::initialize(): 6\n");
			fprintf(stderr, "AVC1394Control::initialize(): node %d\n", i);
//			raw1394_destroy_handle(handle);
			return;
		}
		
		if((rom1394_get_node_type(&rom_dir) == ROM1394_NODE_TYPE_AVC) &&
			avc1394_check_subunit_type(handle, i, AVC1394_SUBUNIT_TYPE_VCR))
		{
//printf("AVC1394Control::initialize(): 7\n");
			device = i;
			break;
		}
	}

	if(device == -1)
	{
//printf("AVC1394Control::initialize(): 8\n");
		fprintf(stderr, "AVC1394Control::initialize(): No AV/C Devices\n");
//		raw1394_destroy_handle(handle);
		return;
	}

}

AVC1394Control::~AVC1394Control()
{
	if(handle) raw1394_destroy_handle(handle);

	if(device_lock) delete device_lock;

}

void AVC1394Control::play()
{
//printf("AVC1394Control::play(): 1\n");
	device_lock->lock("AVC1394Control::play");
	avc1394_vcr_play(handle, device);
	device_lock->unlock();
}

void AVC1394Control::stop()
{
//printf("AVC1394Control::stop(): 1\n");
	device_lock->lock("AVC1394Control::stop");
	avc1394_vcr_stop(handle, device);
	device_lock->unlock();
}

void AVC1394Control::reverse()
{
//printf("AVC1394Control::reverse(): 1\n");
	device_lock->lock("AVC1394Control::reverse");
	avc1394_vcr_reverse(handle, device);
	device_lock->unlock();
}

void AVC1394Control::rewind()
{
//printf("AVC1394Control::rewind(): 1\n");
	device_lock->lock("AVC1394Control::rewind");
	avc1394_vcr_rewind(handle, device);
	device_lock->unlock();
}

void AVC1394Control::fforward()
{
//printf("AVC1394Control::fforward(): 1\n");
	device_lock->lock("AVC1394Control::fforward");
	avc1394_vcr_forward(handle, device);
	device_lock->unlock();
}

void AVC1394Control::pause()
{
//printf("AVC1394Control::pause(): 1\n");
	device_lock->lock("AVC1394Control::pause");
	avc1394_vcr_pause(handle, device);
	device_lock->unlock();
}

void AVC1394Control::record()
{
//printf("AVC1394Control::record(): 1\n");
	device_lock->lock("AVC1394Control::record");
	avc1394_vcr_record(handle, device);
	device_lock->unlock();
}

void AVC1394Control::eject()
{
//printf("AVC1394Control::eject(): 1\n");
	device_lock->lock("AVC1394Control::eject");
	avc1394_vcr_eject(handle, device);
	device_lock->unlock();
}

void AVC1394Control::get_status()
{
//printf("AVC1394Control::get_status(): 1\n");
	device_lock->lock("Control::get_status");
	status = avc1394_vcr_status(handle, device);
	device_lock->unlock();
//	printf("Status: %s\n", avc1394_vcr_decode_status(status));
}

char *AVC1394Control::timecode()
{
	device_lock->lock("AVC1394Control::timecode");
	avc1394_vcr_get_timecode2(handle, device, text_return);
	device_lock->unlock();
	return text_return;
}

void AVC1394Control::seek(char *time)
{
//printf("AVC1394Control::seek(): 1\n");
	device_lock->lock("AVC1394Control::seek");
	avc1394_vcr_seek_timecode(handle, device, time);
	device_lock->unlock();
}
