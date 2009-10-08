
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

#ifndef VDEVICEV4L2JPEG_H
#define VDEVICEV4L2JPEG_H


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_VIDEO4LINUX2
#include "condition.inc"
#include "mutex.inc"
#include "vdevicebase.h"
#include <linux/types.h>
#include <linux/videodev2.h>
#include "videodevice.inc"
#include "vdevicev4l2jpeg.inc"
#include "vdevicev4l2.inc"



class VDeviceV4L2JPEG : public VDeviceBase
{
public:
	VDeviceV4L2JPEG(VideoDevice *device);
	~VDeviceV4L2JPEG();

	int initialize();
	int open_input();
	int close_all();
	int get_best_colormodel(Asset *asset);
	int read_buffer(VFrame *frame);

	VDeviceV4L2Thread *thread;
};

#endif
#endif


