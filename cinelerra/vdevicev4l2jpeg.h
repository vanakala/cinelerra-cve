#ifndef VDEVICEV4L2JPEG_H
#define VDEVICEV4L2JPEG_H

#include "../hvirtual_config.h"

#ifdef HAVE_V4L2
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


