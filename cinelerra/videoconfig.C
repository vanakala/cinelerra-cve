
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

#include "bchash.h"
#include "videoconfig.h"
#include "videodevice.inc"
#include <string.h>

#define CLAMP(x, y, z) (x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))

VideoConfig::VideoConfig()
{
}

VideoConfig::~VideoConfig()
{
}

VideoConfig& VideoConfig::operator=(VideoConfig &that)
{
// Input
	video_in_driver = that.video_in_driver;
	strcpy(v4l_in_device, that.v4l_in_device);
	strcpy(lml_in_device, that.lml_in_device);
	strcpy(screencapture_display, that.screencapture_display);
	vfirewire_in_port = that.vfirewire_in_port;
	vfirewire_in_channel = that.vfirewire_in_channel;
	capture_length = that.capture_length;

// Output
	video_out_driver = that.video_out_driver;
	strcpy(lml_out_device, that.lml_out_device);
	CLAMP(capture_length, 1, 1000);
	return *this;
}

int VideoConfig::load_defaults(BC_Hash *defaults)
{
	video_in_driver = defaults->get("VIDEO_IN_DRIVER", VIDEO4LINUX);
	sprintf(v4l_in_device, "/dev/video");
	defaults->get("V4L_IN_DEVICE", v4l_in_device);
	sprintf(lml_in_device, "/dev/mvideo/stream");
	defaults->get("LML_IN_DEVICE", lml_in_device);
	sprintf(screencapture_display, "");
	defaults->get("SCREENCAPTURE_DISPLAY", screencapture_display);
	vfirewire_in_port = defaults->get("VFIREWIRE_IN_PORT", 0);
	vfirewire_in_channel = defaults->get("VFIREWIRE_IN_CHANNEL", 63);
	capture_length = defaults->get("VIDEO_CAPTURE_LENGTH", 30);

	video_out_driver = defaults->get("VIDEO_OUT_DRIVER", PLAYBACK_X11);
	sprintf(lml_out_device, "/dev/mvideo/stream");
	defaults->get("LML_OUT_DEVICE", lml_out_device);
	return 0;
}

int VideoConfig::save_defaults(BC_Hash *defaults)
{
	defaults->update("VIDEO_IN_DRIVER", video_in_driver);
	defaults->update("V4L_IN_DEVICE", v4l_in_device);
	defaults->update("LML_IN_DEVICE", lml_in_device);
	defaults->update("SCREENCAPTURE_DISPLAY", screencapture_display);
	defaults->update("VFIREWIRE_IN_PORT", vfirewire_in_port);
	defaults->update("VFIREWIRE_IN_CHANNEL", vfirewire_in_channel);
	defaults->update("VIDEO_CAPTURE_LENGTH", capture_length);

	defaults->update("VIDEO_OUT_DRIVER", video_out_driver);
	defaults->update("LML_OUT_DEVICE", lml_out_device);
}
