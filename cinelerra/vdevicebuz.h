#ifndef VDEVICEBUZ_H
#define VDEVICEBUZ_H

#include "buz.h"
#include "channel.inc"
#include "guicast.h"
#include "libmjpeg.h"
#include "mutex.h"
#include "vdevicebase.h"
#include "vframe.inc"

// ./quicktime
#include "jpeg.h"
#include "quicktime.h"

#define INPUT_BUFFER_SIZE 65536

class VDeviceBUZ : public VDeviceBase
{
public:
	VDeviceBUZ(VideoDevice *device);
	~VDeviceBUZ();

	int open_input();
	int open_output();
	int close_all();
	int read_buffer(VFrame *frame);
	int write_buffer(VFrame **frames, EDL *edl);
	int reset_parameters();
	ArrayList<int>* get_render_strategies();
	int set_channel(Channel *channel);
	int get_norm(int norm);
	static void get_inputs(ArrayList<char *> *input_sources);
	int set_picture(int brightness, 
		int hue, 
		int color, 
		int contrast, 
		int whiteness);
	int get_best_colormodel(int colormodel);
	void create_channeldb(ArrayList<Channel*> *channeldb);
	void new_output_buffer(VFrame **outputs, int colormodel);


private:
	int open_input_core(Channel *channel);
	int close_input_core();
	int open_output_core(Channel *channel);
	int close_output_core();

	int jvideo_fd;
	char *input_buffer, *frame_buffer, *output_buffer;
	long input_position;
	long frame_size, frame_allocated;
	int input_error;
//	quicktime_mjpeg_hdr jpeg_header;
	long last_frame_no;
	ArrayList<int> render_strategies;
// Temporary frame for compressing output data
	VFrame *temp_frame;
// Frame given to user to acquire data
	VFrame *user_frame;
	mjpeg_t *mjpeg;
	Mutex tuner_lock;

    struct buz_params bparm;
    struct buz_requestbuffers breq;
    struct buz_sync bsync;
// Can't CSYNC the first loop
	int total_loops;
// Number of output frame to load
	int output_number;
};

#endif
