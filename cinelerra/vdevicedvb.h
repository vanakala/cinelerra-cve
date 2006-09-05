#ifndef VDEVICEDVB_H
#define VDEVICEDVB_H



#include "devicedvbinput.inc"
#include "vdevicebase.h"










class VDeviceDVB : public VDeviceBase
{
public:
	VDeviceDVB(VideoDevice *device);
	~VDeviceDVB();

	int initialize();
	int open_input();
	int close_all();
	int read_buffer(VFrame *frame);
	void fix_asset(Asset *asset);

// Pointer to MWindow::dvb_input
	DeviceDVBInput *input_thread;
};


#endif
