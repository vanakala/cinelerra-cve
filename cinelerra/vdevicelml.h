#ifndef VDEVICELML_H
#define VDEVICELML_H

#include "guicast.h"
#include "vdevicebase.h"

// ./quicktime
#include "jpeg.h"
#include "quicktime.h"

#define INPUT_BUFFER_SIZE 65536

class VDeviceLML : public VDeviceBase
{
public:
	VDeviceLML(VideoDevice *device);
	~VDeviceLML();

	int open_input();
	int open_output();
	int close_all();
	int read_buffer(VFrame *frame);
	int write_buffer(VFrame *frame, EDL *edl);
	int reset_parameters();
	ArrayList<int>* get_render_strategies();

private:
	int reopen_input();

	inline unsigned char get_byte()
	{
		if(!input_buffer) input_buffer = new unsigned char[INPUT_BUFFER_SIZE];
		if(input_position >= INPUT_BUFFER_SIZE) refill_input();
		return input_buffer[input_position++];
	};

	inline unsigned long next_bytes(int total)
	{
		unsigned long result = 0;
		int i;

		if(!input_buffer) input_buffer = new unsigned char[INPUT_BUFFER_SIZE];
		if(input_position + total > INPUT_BUFFER_SIZE) refill_input();

		for(i = 0; i < total; i++)
		{
			result <<= 8;
			result |= input_buffer[input_position + i];
		}
		return result;
	};

	int refill_input();
	inline int write_byte(unsigned char byte)
	{
		if(!frame_buffer)
		{
			frame_buffer = new unsigned char[256000];
			frame_allocated = 256000;
		}

		if(frame_size >= frame_allocated)
		{
			unsigned char *new_frame = new unsigned char[frame_allocated * 2];
			memcpy(new_frame, frame_buffer, frame_size);
			delete frame_buffer;
			frame_buffer = new_frame;
			frame_allocated *= 2;
		}

		frame_buffer[frame_size++] = byte;
		return 0;
	};

	int write_fake_marker();

	FILE *jvideo_fd;
	unsigned char *input_buffer, *frame_buffer;
	long input_position;
	long frame_size, frame_allocated;
	int input_error;
//	quicktime_mjpeg_hdr jpeg_header;
	long last_frame_no;
	ArrayList<int> render_strategies;
};

#endif
