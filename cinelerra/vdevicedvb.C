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




