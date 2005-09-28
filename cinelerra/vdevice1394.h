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


#ifdef HAVE_FIREWIRE



class VDevice1394 : public VDeviceBase
{
public:
	VDevice1394(VideoDevice *device);
	~VDevice1394();

	int open_input();
	int open_output();
	int close_all();
	int read_buffer(VFrame *frame);
	int write_buffer(VFrame **frame, EDL *edl);
// Called by the audio device to share a buffer
//	int get_shared_data(unsigned char *data, long size);
	int initialize();
	int can_copy_from(Asset *asset, int output_w, int output_h);
//	int stop_sharing();
	void new_output_buffer(VFrame **outputs, int colormodel);
	void encrypt(unsigned char *output, unsigned char *data, int data_size);

private:
	Device1394Input *input_thread;
	Device1394Output *output_thread;
	IEC61883Input *input_iec;
	IEC61883Output *output_iec;
	VFrame *user_frame;
};

#endif

#endif
