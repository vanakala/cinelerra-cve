#ifndef VDEVICE1394_H
#define VDEVICE1394_H

#include "guicast.h"
#include "libdv.h"
#include "quicktime.h"
#include "vdevicebase.h"

#define FRAME_BUFFERS 2

#ifdef HAVE_FIREWIRE

#include "video1394.h"

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
	int get_shared_data(unsigned char *data, long size);
	int initialize();
	int can_copy_from(Asset *asset, int output_w, int output_h);
	int stop_sharing();
	int interrupt_crash();
	void new_output_buffer(VFrame **outputs, int colormodel);

private:
	dv_grabber_t *grabber;

// Data allocated in open_input if the device is shared
	VFrame **shared_frames;
	Mutex *shared_output_lock;
// Shared frames being read and written to
	int shared_output_number, shared_input_number;
// The audio device cannot wait if there are no free buffers.  Instead,
// if the next input number is taken it is dropped.
	int *shared_ready;
	int output_fd;
	struct video1394_mmap output_mmap;
	struct video1394_queue_variable output_queue;
	raw1394handle_t avc_handle;
	int avc_id;
	unsigned char *output_buffer;
	int output_number;
    unsigned int packet_sizes[321];
    unsigned char  continuity_counter;
    int unused_buffers;
	VFrame *temp_frame, *temp_frame2, *user_frame;
	dv_t *encoder;
	int cip_n, cip_d;
    int cip_counter;
	char f50_60;
};

#endif

#endif
