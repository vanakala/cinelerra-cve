#ifndef VDEVICEBASE_H
#define VDEVICEBASE_H

#include "assets.inc"
#include "channel.inc"
#include "edl.inc"
#include "guicast.h"
#include "picture.inc"
#include "videodevice.inc"

class VDeviceBase
{
public:
	VDeviceBase(VideoDevice *device);
	virtual ~VDeviceBase();

	virtual int open_input() { return 1; };
	virtual int close_all() { return 1; };
	virtual int read_buffer(VFrame *frame) { return 1; };
	virtual int write_buffer(VFrame **outputs, EDL *edl) { return 1; };
	virtual void new_output_buffer(VFrame **outputs, int colormodel) {};
	virtual ArrayList<int>* get_render_strategies() { return 0; };
	virtual int get_shared_data(unsigned char *data, long size) { return 0; };
	virtual int stop_sharing() { return 0; };
	virtual int interrupt_crash() { return 0; };
// Extra work must sometimes be done in here to set up the device.
	virtual int get_best_colormodel(Asset *asset);
	virtual int set_channel(Channel *channel) { return 0; };
	virtual int set_picture(Picture *picture) { return 0; };

	virtual int open_output() { return 1; };
	virtual int output_visible() { return 0; };
	virtual int start_playback() { return 1; };
	virtual int stop_playback() { return 1; };
	virtual BC_Bitmap* get_bitmap() { return 0; };
// Most Linux video drivers don't work.
// Called by KeepaliveThread when the device appears to be stuck.
// Should restart the device if that's what it takes to get it to work.
	virtual void goose_input() {};

	VideoDevice *device;
};

#endif
