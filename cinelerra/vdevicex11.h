#ifndef VDEVICEX11_H
#define VDEVICEX11_H

#include "canvas.inc"
#include "edl.inc"
#include "guicast.h"
#include "thread.h"
#include "vdevicebase.h"

// output_frame is the same one written to device
#define BITMAP_PRIMARY 0
// output_frame is a temporary converted to the device format
#define BITMAP_TEMP    1

class VDeviceX11 : public VDeviceBase
{
public:
	VDeviceX11(VideoDevice *device, Canvas *output);
	~VDeviceX11();

	int open_input();
	int close_all();
	int read_buffer(VFrame *frame);
	int reset_parameters();
// User always gets the colormodel requested
	void new_output_buffer(VFrame **output_frames, int colormodel);

	int open_output();
	int start_playback();
	int stop_playback();
	int output_visible();
// After loading the bitmap with a picture, write it
	int write_buffer(VFrame **outputs, EDL *edl);

private:
// Closest colormodel the hardware can do for playback
	int get_best_colormodel(int colormodel);
// Get best colormodel for recording
	int get_best_colormodel(Asset *asset);

	BC_Bitmap *bitmap;        // Bitmap to be written to device
	VFrame *output_frame;     // Wrapper for bitmap or intermediate frame
	int bitmap_type;           // Type of output_frame
	int bitmap_w, bitmap_h;   // dimensions of buffers written to window
	ArrayList<int> render_strategies;
// Canvas for output
	Canvas *output;
	int color_model;
	int color_model_selected;
// Transfers for last frame rendered.
// These stick the last frame to the display.
	int in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h;
// Screen capture
	BC_Capture *capture_bitmap;
};

#endif
