// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VIDEODEVICE_INC
#define VIDEODEVICE_INC

class VideoDevice;

// Flags for mainwindow->preferences->vconfig_in->driver
#define VIDEO4LINUX          0
#define VIDEO4LINUX2         1
#define VIDEO4LINUX2JPEG     2
#define SCREENCAPTURE        3
#define CAPTURE_DVB          8

#define VIDEO4LINUX_TITLE      "Video4Linux"
#define VIDEO4LINUX2_TITLE     "Video4Linux2"
#define VIDEO4LINUX2JPEG_TITLE "Video4Linux2 JPEG"
#define SCREENCAPTURE_TITLE    "Screencapture"
#define CAPTURE_DVB_TITLE      "DVB"

#define PLAYBACK_X11 	  10
#define PLAYBACK_X11_XV   14
#define PLAYBACK_X11_GL   17

// This is passed to File::get_best_colormodel by FileThread so FileThread can
// do the colorspace conversion using the shortest path.
#define PLAYBACK_ASYNCHRONOUS 18

#define PLAYBACK_X11_TITLE  	"X11"
#define PLAYBACK_X11_XV_TITLE   "X11-XV"
#define PLAYBACK_X11_GL_TITLE   "X11-OpenGL"

// Output formats required for direct frame copy
// RGB is for either RGB or RGBA as defined in the VFrame
#define OUTPUT_RGB  0
#define OUTPUT_MJPA 1
#define OUTPUT_DV   2

#endif
