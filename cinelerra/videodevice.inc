
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
