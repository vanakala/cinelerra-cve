
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

#ifndef VDEVICEV4L_H
#define VDEVICEV4L_H

#include "vdevicebase.h"
#include <linux/videodev.h>
#include "videodevice.inc"

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
	int set_picture(PictureConfig *picture);

private:
	int set_cloexec_flag(int desc, int value);
	int set_mute(int muted);
	int v4l1_get_inputs();
	int v4l1_set_mute(int muted);
	unsigned long translate_colormodel(int colormodel);
	int v4l1_set_channel(Channel *channel);
	int v4l1_get_norm(int norm);
	int v4l1_set_picture(PictureConfig *picture);
	void v4l1_start_capture();
	int capture_frame(int capture_frame_number);
	int wait_v4l_frame();
	int read_v4l_frame(VFrame *frame);
	int frame_to_vframe(VFrame *frame, unsigned char *input);
	int next_frame(int previous_frame);
	int close_v4l();
	int unmap_v4l_shmem();
	int v4l_init();

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

// Common
	char *capture_buffer;      // sequentual capture buffers for v4l1 or read buffer for v4l2
	int capture_frame_number;    // number of frame to capture into
	int read_frame_number;       // number of the captured frame to read
	int shared_memory;   // Capturing directly to memory
	int initialization_complete;
	int got_first_frame;
};

#endif
