// ALPHA C++ can't compile 64 bit headers
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE
#undef _FILE_OFFSET_BITS

#include "assets.h"
#include "channel.h"
#include "chantables.h"
#include "file.inc"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "strategies.inc"
#include "vdevicebuz.h"
#include "vframe.h"
#include "videoconfig.h"
#include "videodevice.h"

#include <errno.h>
#include <linux/kernel.h>
#include "videodev2.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SOI 0xffd8
#define APP3 0xffe3
#define APP1 0xffe1
#define APP0 0xffe0
#define EOI  0xffd9

VDeviceBUZ::VDeviceBUZ(VideoDevice *device)
 : VDeviceBase(device)
{
	reset_parameters();
	render_strategies.append(VRENDER_MJPG);
}

VDeviceBUZ::~VDeviceBUZ()
{
//printf("VDeviceBUZ::~VDeviceBUZ 1\n");
	close_all();
//printf("VDeviceBUZ::~VDeviceBUZ 2\n");
}

int VDeviceBUZ::reset_parameters()
{
	jvideo_fd = 0;
	input_buffer = 0;
	output_buffer = 0;
	frame_buffer = 0;
	frame_size = 0;
	frame_allocated = 0;
	input_error = 0;
	input_position = INPUT_BUFFER_SIZE;
	last_frame_no = 0;
	temp_frame = 0;
	user_frame = 0;
	mjpeg = 0;
	total_loops = 0;
	output_number = 0;
}

int VDeviceBUZ::close_input_core()
{
//printf("VDeviceBUZ::close_input_core 1\n");

	if(device->r)
	{
		if(jvideo_fd) close(jvideo_fd);
		jvideo_fd = 0;
	}

	if(input_buffer)
	{
		if(input_buffer > 0)
			munmap(input_buffer, breq.count * breq.size);
		input_buffer = 0;
	}

//printf("VDeviceBUZ::close_input_core 2\n");
}

int VDeviceBUZ::close_output_core()
{
//printf("VDeviceBUZ::close_output_core 1\n");
	if(device->w)
	{
		int n = -1;
// 		if(ioctl(jvideo_fd, BUZIOC_QBUF_PLAY, &n) < 0)
// 			perror("VDeviceBUZ::close_output_core BUZIOC_QBUF_PLAY");
		if(jvideo_fd) close(jvideo_fd);
		jvideo_fd = 0;
	}
	if(output_buffer)
	{
		if(output_buffer > 0)
			munmap(output_buffer, breq.count * breq.size);
		output_buffer = 0;
	}
	if(temp_frame)
	{
		delete temp_frame;
		temp_frame = 0;
	}
	if(mjpeg)
	{
		mjpeg_delete(mjpeg);
		mjpeg = 0;
	}
	if(user_frame)
	{
		delete user_frame;
		user_frame = 0;
	}
//printf("VDeviceBUZ::close_output_core 2\n");
	return 0;
}


int VDeviceBUZ::close_all()
{
//printf("VDeviceBUZ::close_all 1\n");
	close_input_core();
//printf("VDeviceBUZ::close_all 1\n");
	close_output_core();
//printf("VDeviceBUZ::close_all 1\n");
	if(frame_buffer) delete frame_buffer;
//printf("VDeviceBUZ::close_all 1\n");
	reset_parameters();
//printf("VDeviceBUZ::close_all 2\n");
	return 0;
}

#define COMPOSITE_TEXT "Composite"
#define SVIDEO_TEXT "S-Video"
#define BUZ_COMPOSITE 0
#define BUZ_SVIDEO 1

void VDeviceBUZ::get_inputs(ArrayList<char *> *input_sources)
{
	char *new_source;
	input_sources->append(new_source = new char[strlen(COMPOSITE_TEXT) + 1]);
	strcpy(new_source, COMPOSITE_TEXT);
	input_sources->append(new_source = new char[strlen(SVIDEO_TEXT) + 1]);
	strcpy(new_source, SVIDEO_TEXT);
}

int VDeviceBUZ::open_input()
{
// Can't open input until after the channel is set
	return 0;
}

int VDeviceBUZ::open_output()
{
// Can't open output until after the channel is set
	return 0;
}

int VDeviceBUZ::set_channel(Channel *channel)
{
//printf("VDeviceBUZ::set_channel 1\n");
	if(!channel) return 0;

	tuner_lock.lock();
	if(device->r)
	{
		close_input_core();
		open_input_core(channel);
	}
	else
	{
		close_output_core();
		open_output_core(channel);
	}
	tuner_lock.unlock();
//printf("VDeviceBUZ::set_channel 2\n");


	return 0;
}

void VDeviceBUZ::create_channeldb(ArrayList<Channel*> *channeldb)
{
	;
}

int VDeviceBUZ::set_picture(int brightness, 
		int hue, 
		int color, 
		int contrast, 
		int whiteness)
{
//printf("VDeviceBUZ::set_picture %d\n", color);
	tuner_lock.lock();

	struct video_picture picture_params;
	brightness = (int)((float)brightness / 100 * 32767 + 32768);
	hue = (int)((float)hue / 100 * 32767 + 32768);
	color = (int)((float)color / 100 * 32767 + 32768);
	contrast = (int)((float)contrast / 100 * 32767 + 32768);
	whiteness = (int)((float)whiteness / 100 * 32767 + 32768);
	if(ioctl(jvideo_fd, VIDIOCGPICT, &picture_params) < 0)
		perror("VDeviceBUZ::set_picture VIDIOCGPICT");
	picture_params.brightness = brightness;
	picture_params.hue = hue;
	picture_params.colour = color;
	picture_params.contrast = contrast;
	picture_params.whiteness = whiteness;
	if(ioctl(jvideo_fd, VIDIOCSPICT, &picture_params) < 0)
		perror("VDeviceBUZ::set_picture VIDIOCSPICT");
	if(ioctl(jvideo_fd, VIDIOCGPICT, &picture_params) < 0)
		perror("VDeviceBUZ::set_picture VIDIOCGPICT");




	tuner_lock.unlock();
	return 0;
}

int VDeviceBUZ::get_norm(int norm)
{
	switch(norm)
	{
		case NTSC:          return VIDEO_MODE_NTSC;      break;
		case PAL:           return VIDEO_MODE_PAL;       break;
		case SECAM:         return VIDEO_MODE_SECAM;     break;
	}
}

int VDeviceBUZ::read_buffer(VFrame *frame)
{
	tuner_lock.lock();
	if(!jvideo_fd) open_input_core(0);

//printf("VDeviceBUZ::read_buffer 1 %d\n", bsync.frame);
	if(ioctl(jvideo_fd, BUZIOC_SYNC, &bsync) < 0)
		perror("VDeviceBUZ::read_buffer BUZIOC_SYNC");
	else
	{
//printf("VDeviceBUZ::read_buffer 2 %d\n", bsync.frame);
// Give 0xc0 extra for markers
		frame->allocate_compressed_data(bsync.length + 0xc0);
//printf("VDeviceBUZ::read_buffer 2 %d\n", bsync.frame);
		frame->set_compressed_size(bsync.length);
//printf("VDeviceBUZ::read_buffer 3 %d\n", bsync.frame);

		char *buffer = input_buffer + bsync.frame * breq.size;
// Transfer fields to frame
//fwrite(buffer, bsync.length, 1, stdout);
//printf("VDeviceBUZ::read_buffer 4 %d\n", bsync.length);
		if(device->odd_field_first)
		{
//printf("VDeviceBUZ::read_buffer 5 %d\n", bsync.length);
			long field2_offset = mjpeg_get_field2((unsigned char*)buffer, bsync.length);
//printf("VDeviceBUZ::read_buffer 6 %d\n", bsync.length);
			long field1_len = field2_offset;
//printf("VDeviceBUZ::read_buffer 7 %d\n", bsync.length);
			long field2_len = bsync.length - field2_offset;
//printf("VDeviceBUZ::read_buffer 8.1 %d\n", bsync.frame);

			memcpy(frame->get_data(), buffer + field2_offset, field2_len);
//printf("VDeviceBUZ::read_buffer 9.2 %d\n", bsync.frame);
			memcpy(frame->get_data() + field2_len, buffer, field1_len);
//printf("VDeviceBUZ::read_buffer 10.3 %d\n", bsync.frame);
// printf("VDeviceBUZ::read_buffer %d %02x%02x %02x%02x\n", 
// 	field2_len, 
// 	frame->get_data()[0], 
// 	frame->get_data()[1],
// 	frame->get_data()[field2_len], 
// 	frame->get_data()[field2_len + 1]);
		}
		else
		{
			bcopy(buffer, frame->get_data(), bsync.length);
		}
	}
//printf("VDeviceBUZ::read_buffer 11 %d\n", bsync.frame);

	if(ioctl(jvideo_fd, BUZIOC_QBUF_CAPT, &bsync.frame))
		perror("VDeviceBUZ::read_buffer BUZIOC_QBUF_CAPT");
//printf("VDeviceBUZ::read_buffer 12 %d\n", bsync.frame);

	tuner_lock.unlock();
	return 0;
}

int VDeviceBUZ::open_input_core(Channel *channel)
{
//printf("VDeviceBUZ::open_input_core 1\n");
	jvideo_fd = open(device->in_config->buz_in_device, O_RDONLY);

	if(jvideo_fd <= 0)
	{
		fprintf(stderr, "VDeviceBUZ::open_input %s: %s\n", 
			device->in_config->buz_in_device,
			strerror(errno));
		jvideo_fd = 0;
		return 1;
	}

// Create input sources
	get_inputs(&device->input_sources);

// Set current input source
	if(channel)
	{
		for(int i = 0; i < 2; i++)
		{
			struct video_channel vch;
			vch.channel = channel->input;
			vch.norm = get_norm(channel->norm);

//printf("VDeviceBUZ::open_input_core 2 %d %d\n", vch.channel, vch.norm);
			if(ioctl(jvideo_fd, VIDIOCSCHAN, &vch) < 0)
				perror("VDeviceBUZ::open_input_core VIDIOCSCHAN ");
		}
	}


// Throw away
    struct video_capability vc;
	if(ioctl(jvideo_fd, VIDIOCGCAP, &vc) < 0)
		perror("VDeviceBUZ::open_input VIDIOCGCAP");

// API dependant initialization
	if(ioctl(jvideo_fd, BUZIOC_G_PARAMS, &bparm) < 0)
		perror("VDeviceBUZ::open_input BUZIOC_G_PARAMS");

	bparm.HorDcm = 1;
	bparm.VerDcm = 1;
	bparm.TmpDcm = 1;
	bparm.field_per_buff = 2;
	bparm.img_width = device->in_config->w;
	bparm.img_height = device->in_config->h / bparm.field_per_buff;
	bparm.img_x = 0;
	bparm.img_y = 0;
//	bparm.APPn = 0;
//	bparm.APP_len = 14;
	bparm.APP_len = 0;
	bparm.odd_even = 0;
    bparm.decimation = 0;
    bparm.quality = device->quality;
    bzero(bparm.APP_data, sizeof(bparm.APP_data));

	if(ioctl(jvideo_fd, BUZIOC_S_PARAMS, &bparm) < 0)
		perror("VDeviceBUZ::open_input BUZIOC_S_PARAMS");

// printf("open_input %d %d %d %d %d %d %d %d %d %d %d %d\n", 
// 		bparm.HorDcm,
// 		bparm.VerDcm,
// 		bparm.TmpDcm,
// 		bparm.field_per_buff,
// 		bparm.img_width,
// 		bparm.img_height,
// 		bparm.img_x,
// 		bparm.img_y,
// 		bparm.APP_len,
// 		bparm.odd_even,
//     	bparm.decimation,
//     	bparm.quality);

	breq.count = device->in_config->capture_length;
	breq.size = 0x40000;
	if(ioctl(jvideo_fd, BUZIOC_REQBUFS, &breq) < 0)
		perror("VDeviceBUZ::open_input BUZIOC_REQBUFS");

//printf("open_input %s %d %d %d %d\n", device->in_config->buz_in_device, breq.count, breq.size, bparm.img_width, bparm.img_height);
	if((input_buffer = (char*)mmap(0, 
		breq.count * breq.size, 
		PROT_READ, 
		MAP_SHARED, 
		jvideo_fd, 
		0)) <= 0)
		perror("VDeviceBUZ::open_input mmap");

// Start capturing
	for(int i = 0; i < breq.count; i++)
	{
        if(ioctl(jvideo_fd, BUZIOC_QBUF_CAPT, &i) < 0)
			perror("VDeviceBUZ::open_input BUZIOC_QBUF_CAPT");
	}

//printf("VDeviceBUZ::open_input_core 2\n");
	return 0;
}

int VDeviceBUZ::open_output_core(Channel *channel)
{
//printf("VDeviceBUZ::open_output 1\n");
	total_loops = 0;
	output_number = 0;
	jvideo_fd = open(device->out_config->buz_out_device, O_RDWR);
	if(jvideo_fd <= 0)
	{
		perror("VDeviceBUZ::open_output");
		return 1;
	}


// Set current input source
	if(channel)
	{
		struct video_channel vch;
		vch.channel = channel->input;
		vch.norm = get_norm(channel->norm);

//printf("VDeviceBUZ::open_input_core 2 %d %d\n", vch.channel, vch.norm);
		if(ioctl(jvideo_fd, VIDIOCSCHAN, &vch) < 0)
			perror("VDeviceBUZ::open_output_core VIDIOCSCHAN ");
	}

	breq.count = 10;
	breq.size = 0x40000;
	if(ioctl(jvideo_fd, BUZIOC_REQBUFS, &breq) < 0)
		perror("VDeviceBUZ::open_output BUZIOC_REQBUFS");
	if((output_buffer = (char*)mmap(0, 
		breq.count * breq.size, 
		PROT_READ | PROT_WRITE, 
		MAP_SHARED, 
		jvideo_fd, 
		0)) <= 0)
		perror("VDeviceBUZ::open_output mmap");

	if(ioctl(jvideo_fd, BUZIOC_G_PARAMS, &bparm) < 0)
		perror("VDeviceBUZ::open_output BUZIOC_G_PARAMS");

	bparm.decimation = 1;
	bparm.HorDcm = 1;
	bparm.field_per_buff = 2;
	bparm.TmpDcm = 1;
	bparm.VerDcm = 1;
	bparm.img_width = device->out_w;
	bparm.img_height = device->out_h / bparm.field_per_buff;
	bparm.img_x = 0;
	bparm.img_y = 0;
	bparm.odd_even = 0;

	if(ioctl(jvideo_fd, BUZIOC_S_PARAMS, &bparm) < 0)
		perror("VDeviceBUZ::open_output BUZIOC_S_PARAMS");
//printf("VDeviceBUZ::open_output 2\n");
	return 0;
}



int VDeviceBUZ::write_buffer(VFrame **frames, EDL *edl)
{
//printf("VDeviceBUZ::write_buffer 1\n");
	tuner_lock.lock();

	if(!jvideo_fd) open_output_core(0);

	VFrame *ptr = 0;
	if(frames[0]->get_color_model() != BC_COMPRESSED)
	{
		if(!temp_frame) temp_frame = new VFrame;
		if(!mjpeg)
		{
			mjpeg = mjpeg_new(device->out_w, device->out_h, 2);
			mjpeg_set_quality(mjpeg, device->quality);
			mjpeg_set_float(mjpeg, 0);
		}
		ptr = temp_frame;
		mjpeg_compress(mjpeg, 
			frames[0]->get_rows(), 
			frames[0]->get_y(), 
			frames[0]->get_u(), 
			frames[0]->get_v(),
			frames[0]->get_color_model(),
			device->cpus);
		temp_frame->allocate_compressed_data(mjpeg_output_size(mjpeg));
		temp_frame->set_compressed_size(mjpeg_output_size(mjpeg));
		bcopy(mjpeg_output_buffer(mjpeg), temp_frame->get_data(), mjpeg_output_size(mjpeg));
	}
	else
		ptr = frames[0];

// Wait for frame to become available
// Caused close_output_core to lock up.
// 	if(total_loops >= 1)
// 	{
// 		if(ioctl(jvideo_fd, BUZIOC_SYNC, &output_number) < 0)
// 			perror("VDeviceBUZ::write_buffer BUZIOC_SYNC");
// 	}

	if(device->out_config->buz_swap_fields)
	{
		long field2_offset = mjpeg_get_field2((unsigned char*)ptr->get_data(), 
			ptr->get_compressed_size());
		long field2_len = ptr->get_compressed_size() - field2_offset;
		memcpy(output_buffer + output_number * breq.size, 
			ptr->get_data() + field2_offset, 
			field2_len);
		memcpy(output_buffer + output_number * breq.size +field2_len,
			ptr->get_data(), 
			field2_offset);
	}
	else
	{
		bcopy(ptr->get_data(), 
			output_buffer + output_number * breq.size, 
			ptr->get_compressed_size());
	}

	if(ioctl(jvideo_fd, BUZIOC_QBUF_PLAY, &output_number) < 0)
		perror("VDeviceBUZ::write_buffer BUZIOC_QBUF_PLAY");

	output_number++;
	if(output_number >= breq.count)
	{
		output_number = 0;
		total_loops++;
	}
	tuner_lock.unlock();
//printf("VDeviceBUZ::write_buffer 2\n");

	return 0;
}

void VDeviceBUZ::new_output_buffer(VFrame **outputs,
	int colormodel)
{
//printf("VDeviceBUZ::new_output_buffer 1 %d\n", colormodel);
	if(user_frame)
	{
		if(colormodel != user_frame->get_color_model())
		{
			delete user_frame;
			user_frame = 0;
		}
	}

	if(!user_frame)
	{
		switch(colormodel)
		{
			case BC_COMPRESSED:
				user_frame = new VFrame;
				break;
			default:
				user_frame = new VFrame(0,
					device->out_w,
					device->out_h,
					colormodel,
					-1);
				break;
		}
	}
	user_frame->set_shm_offset(0);
	outputs[0] = user_frame;
//printf("VDeviceBUZ::new_output_buffer 2\n");
}


ArrayList<int>* VDeviceBUZ::get_render_strategies()
{
	return &render_strategies;
}

