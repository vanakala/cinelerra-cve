#ifndef VDEVICEV4L_H
#define VDEVICEV4L_H

#include "vdevicebase.h"
#include <linux/kernel.h>
#include "videodevice.inc"
#include "videodev2.h"

// Structure for video4linux2
struct tag_vimage
{
	struct v4l2_buffer vidbuf;
	char *data;
};

class VDeviceV4L : public VDeviceBase
{
public:
	VDeviceV4L(VideoDevice *device);
	~VDeviceV4L();

	int initialize();
	int open_input();
	int close_all();
	int read_buffer(VFrame *frame);
	int get_best_colormodel(Asset *asset);
	int set_channel(Channel *channel);
	int set_picture(int brightness, 
		int hue, 
		int color, 
		int contrast, 
		int whiteness);

private:
	int set_cloexec_flag(int desc, int value);
	int set_mute(int muted);
	int v4l1_get_inputs();
	int v4l1_set_mute(int muted);
	unsigned long translate_colormodel(int colormodel);
	int v4l1_set_channel(Channel *channel);
	int v4l1_get_norm(int norm);
	int v4l1_set_picture(int brightness, 
		int hue, 
		int color, 
		int contrast, 
		int whiteness);
	void v4l1_start_capture();
	int capture_frame(int capture_frame_number);
	int wait_v4l_frame();
	int wait_v4l2_frame();
	int read_v4l_frame(VFrame *frame);
	int read_v4l2_frame(VFrame *frame);
	void v4l2_start_capture();
	int frame_to_vframe(VFrame *frame, unsigned char *input);
	int next_frame(int previous_frame);
	int close_v4l();
	int close_v4l2();
	int unmap_v4l_shmem();
	int unmap_v4l2_shmem();
	int v4l_init();
	int v4l2_init();

	int derivative;
	int input_fd, output_fd;
// FourCC Colormodel for device
	unsigned long device_colormodel;
// BC colormodel for device
	int colormodel;

// Video4Linux
	struct video_capability cap1;
	struct video_window window_params;
	struct video_picture picture_params;
	struct video_mbuf capture_params;  // Capture for Video4Linux

// Video4Linux 2
	struct v4l2_capability cap2;
	struct v4l2_requestbuffers v4l2_buffers;
	struct v4l2_format v4l2_params;
	struct tag_vimage *v4l2_buffer_list;
	struct v4l2_streamparm v4l2_parm;

// Common
	char *capture_buffer;      // sequentual capture buffers for v4l1 or read buffer for v4l2
	int capture_frame_number;    // number of frame to capture into
	int read_frame_number;       // number of the captured frame to read
	int shared_memory;   // Capturing directly to memory
	int initialization_complete;
	int got_first_frame;
};

#endif
