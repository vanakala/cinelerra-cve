// V4L2 is incompatible with large file support
// ALPHA C++ can't compile 64 bit headers
#undef _FILE_OFFSET_BITS
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE


#include "assets.h"
#include "channel.h"
#include "chantables.h"
#include "clip.h"
#include "file.h"
#include "preferences.h"
#include "quicktime.h"
#include "recordconfig.h"
#include "vdevicev4l.h"
#include "vframe.h"
#include "videodevice.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

VDeviceV4L::VDeviceV4L(VideoDevice *device)
 : VDeviceBase(device)
{
	initialize();
}

VDeviceV4L::~VDeviceV4L()
{
}

int VDeviceV4L::initialize()
{
	derivative = VIDEO4LINUX;
	capture_buffer = 0;
	capture_frame_number = 0;
	read_frame_number = 0;
	shared_memory = 0;
	v4l2_buffer_list = 0;
	initialization_complete = 0;
	return 0;
}

int VDeviceV4L::open_input()
{
//printf("VDeviceV4L::open_input 1\n");
// Get the video4linux derivative
	if((input_fd = open(device->in_config->v4l_in_device, O_RDWR)) < 0)
	{
		perror("VDeviceV4L::open_input");
		return 1;
	}
	else
	{
//printf("VDeviceV4L::open_input 1\n");
		if(ioctl(input_fd, VIDIOC_QUERYCAP, &cap2) != -1)
		{
			derivative = VIDEO4LINUX2;
		}
		else
		if(ioctl(input_fd, VIDIOCGCAP, &cap1) != -1)
		{
			derivative = VIDEO4LINUX;
		}
//printf("VDeviceV4L::open_input 1\n");
		v4l1_get_inputs();
//printf("VDeviceV4L::open_input 1\n");
		close(input_fd);
//printf("VDeviceV4L::open_input 2\n");
	}
	return 0;
}

int VDeviceV4L::close_all()
{
	switch(derivative)
	{
		case VIDEO4LINUX:
			close_v4l();
			break;
		case VIDEO4LINUX2:
			close_v4l2();
			break;
	}
	return 0;
}

int VDeviceV4L::close_v4l()
{
	unmap_v4l_shmem();
	if(input_fd != -1) close(input_fd);
	return 0;
}

int VDeviceV4L::close_v4l2()
{
	if(v4l2_buffer_list)
	{
		int temp = V4L2_BUF_TYPE_CAPTURE;
		if(ioctl(input_fd, VIDIOC_STREAMOFF, &temp) < 0)
			perror("VDeviceV4L::close_v4l2 VIDIOC_STREAMOFF");
	}
	unmap_v4l2_shmem();
	if(input_fd != -1) close(input_fd);
	return 0;
}

int VDeviceV4L::unmap_v4l_shmem()
{
	if(capture_buffer)
	{
		if(shared_memory)
			munmap(capture_buffer, capture_params.size);
		else
			delete capture_buffer;
	}
	return 0;
}

int VDeviceV4L::unmap_v4l2_shmem()
{
	if(capture_buffer)
	{
		delete capture_buffer;
		capture_buffer = 0;
	}
	else
	if(v4l2_buffer_list)
	{
		for(int i = 0; i < v4l2_buffers.count; i++)
		{
			if((long)v4l2_buffer_list[i].data != -1)
				munmap(v4l2_buffer_list[i].data, 
					   v4l2_buffer_list[i].vidbuf.length);
		}
		delete v4l2_buffer_list;
		v4l2_buffer_list = 0;
	}
	return 0;
}

int VDeviceV4L::v4l_init()
{
	int i;

	input_fd = open(device->in_config->v4l_in_device, O_RDWR);

	if(input_fd < 0)
		perror("VDeviceV4L::init_video4linux");
	else
	{
		set_cloexec_flag(input_fd, 1);
		set_mute(0);
		if(ioctl(input_fd, VIDIOCGWIN, &window_params) < 0)
			perror("VDeviceV4L::v4l_init VIDIOCGWIN");
		window_params.x = 0;
		window_params.y = 0;
//printf("VDeviceV4L::v4l_init 1 %d %d\n", device->in_config->w, device->in_config->h );
		window_params.width = device->in_config->w;
		window_params.height = device->in_config->h;
		window_params.chromakey = 0;
		window_params.flags = 0;
		window_params.clipcount = 0;
		if(ioctl(input_fd, VIDIOCSWIN, &window_params) < 0)
			perror("VDeviceV4L::v4l_init VIDIOCSWIN");
		if(ioctl(input_fd, VIDIOCGWIN, &window_params) < 0)
			perror("VDeviceV4L::v4l_init VIDIOCGWIN");

//printf("VDeviceV4L::v4l_init 2 %d %d\n", device->in_config->w, device->in_config->h );
		device->in_config->w = window_params.width;
		device->in_config->h = window_params.height;

		set_picture(-1, -1, -1, -1, -1);

		if(ioctl(input_fd, VIDIOCGMBUF, &capture_params) < 0)
			perror("VDeviceV4L::v4l_init VIDIOCGMBUF");

		capture_buffer = (char*)mmap(0, 
			capture_params.size, 
			PROT_READ|PROT_WRITE, 
			MAP_SHARED, 
			input_fd, 
			0);
//printf("VDeviceV4L::v4l_init 2 %d\n", capture_params.size);

		capture_frame_number = 0;

		if((long)capture_buffer < 0)
		{
// Use read instead.
			perror("VDeviceV4L::v4l_init mmap");
			shared_memory = 0;
			capture_buffer = new char[capture_params.size];
		}
		else
		{
// Get all frames capturing
			shared_memory = 1;
// 			for(i = 0; i < capture_params.frames; i++)
		}
	}
	got_first_frame = 0;
	return 0;
}

void VDeviceV4L::v4l1_start_capture()
{
	for(int i = 0; i < MIN(capture_params.frames, device->in_config->capture_length); i++)
		capture_frame(i);
}






int VDeviceV4L::v4l2_init()
{
	int i;
	v4l2_params.type = V4L2_BUF_TYPE_CAPTURE;

	close_v4l2();
	input_fd = open(device->in_config->v4l_in_device, O_RDONLY);

	if(input_fd < 0)
		perror("VDeviceV4L::v4l2_init");
	else
	{
		struct v4l2_capability cap;

		set_cloexec_flag(input_fd, 1);
		if(ioctl(input_fd, VIDIOC_QUERYCAP, &cap))
			perror("VDeviceV4L::v4l2_init VIDIOC_QUERYCAP");

		v4l2_parm.type = V4L2_BUF_TYPE_CAPTURE;
// Set up frame rate
		if(ioctl(input_fd, VIDIOC_G_PARM, &v4l2_parm))
			perror("VDeviceV4L::v4l2_init VIDIOC_G_PARM");

		if(v4l2_parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
		{
			v4l2_parm.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
			v4l2_parm.parm.capture.timeperframe = (unsigned long)((float)1 / device->frame_rate * 10000000);
			if(ioctl(input_fd, VIDIOC_S_PARM, &v4l2_parm))
				perror("VDeviceV4L::v4l2_init VIDIOC_S_PARM");

			if(ioctl(input_fd, VIDIOC_G_PARM, &v4l2_parm))
				perror("VDeviceV4L::v4l2_init VIDIOC_G_PARM");
		}

// Set up data format
		if(ioctl(input_fd, VIDIOC_G_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L::v4l2_init VIDIOC_G_FMT");
		v4l2_params.fmt.pix.width = device->in_config->w;
		v4l2_params.fmt.pix.height = device->in_config->h;
		v4l2_params.fmt.pix.depth = 24;
		v4l2_params.fmt.pix.pixelformat = device_colormodel;
		v4l2_params.fmt.pix.flags |= V4L2_FMT_FLAG_INTERLACED;
		if(ioctl(input_fd, VIDIOC_S_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L::v4l2_init VIDIOC_S_FMT");
		if(ioctl(input_fd, VIDIOC_G_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L::v4l2_init VIDIOC_G_FMT");

		device->in_config->w = v4l2_params.fmt.pix.width;
		device->in_config->h = v4l2_params.fmt.pix.height;

// Set up buffers
		if(cap.flags & V4L2_FLAG_STREAMING)
		{
// Can use streaming
			shared_memory = 1;
			v4l2_buffers.count = device->in_config->capture_length;
			v4l2_buffers.type = V4L2_BUF_TYPE_CAPTURE;
			if(ioctl(input_fd, VIDIOC_REQBUFS, &v4l2_buffers) < 0)
				perror("VDeviceV4L::v4l2_init VIDIOC_REQBUFS");

			v4l2_buffer_list = new struct tag_vimage[v4l2_buffers.count];
			for(i = 0; i < v4l2_buffers.count; i++)
			{
				v4l2_buffer_list[i].vidbuf.index = i;
				v4l2_buffer_list[i].vidbuf.type = V4L2_BUF_TYPE_CAPTURE;
				if(ioctl(input_fd, VIDIOC_QUERYBUF, &v4l2_buffer_list[i].vidbuf) < 0)
					perror("VDeviceV4L::v4l2_init VIDIOC_QUERYBUF");
				v4l2_buffer_list[i].data = (char*)mmap(0, 
								v4l2_buffer_list[i].vidbuf.length, 
								PROT_READ,
				    		  	MAP_SHARED, 
								input_fd, 
				    		  	v4l2_buffer_list[i].vidbuf.offset);

				if((long)v4l2_buffer_list[i].data == -1)
					perror("VDeviceV4L::v4l2_init mmap");
			}
// Start all frames capturing
		}
		else
		{
// Need to use read()
			shared_memory = 0;
			capture_buffer = new char[v4l2_params.fmt.pix.sizeimage];
		}

		capture_frame_number = 0;
	}
	got_first_frame = 0;
	return 0;
}


void VDeviceV4L::v4l2_start_capture()
{
	for(int i = 0; i < v4l2_buffers.count; i++)
	{
		if(ioctl(input_fd, VIDIOC_QBUF, &v4l2_buffer_list[i].vidbuf) < 0)
			perror("VDeviceV4L::v4l2_init VIDIOC_QBUF");
	}
	if(ioctl(input_fd, VIDIOC_STREAMON, &v4l2_buffer_list[0].vidbuf.type) < 0)
		perror("VDeviceV4L::v4l2_init VIDIOC_STREAMON");
}



int VDeviceV4L::v4l1_get_inputs()
{
	struct video_channel channel_struct;
	int i = 0, done = 0;
	char *new_source;

//printf("VDeviceV4L::v4l1_get_inputs 1\n");
	while(!done && i < 20)
	{
		channel_struct.channel = i;
//printf("VDeviceV4L::v4l1_get_inputs 2\n");
		if(ioctl(input_fd, VIDIOCGCHAN, &channel_struct) < 0)
		{
// Finished
			done = 1;
		}
		else
		{
//printf("VDeviceV4L::v4l1_get_inputs 3 %s\n", channel_struct.name);
			device->input_sources.append(new_source = new char[strlen(channel_struct.name) + 1]);
//printf("VDeviceV4L::v4l1_get_inputs 4\n");
			strcpy(new_source, channel_struct.name);
//printf("VDeviceV4L::v4l1_get_inputs 5\n");
		}
		i++;
//printf("VDeviceV4L::v4l1_get_inputs 6\n");
	}
//printf("VDeviceV4L::v4l1_get_inputs 7\n");
	return 0;
}

int VDeviceV4L::set_mute(int muted)
{
// Open audio, which obviously is controlled by the video driver.
// and apparently resets the input source.
	if(derivative == VIDEO4LINUX)
	{
		v4l1_set_mute(muted);
	}
}

int VDeviceV4L::v4l1_set_mute(int muted)
{
	struct video_audio audio;

    if(ioctl(input_fd, VIDIOCGAUDIO, &audio))
	if(ioctl(input_fd, VIDIOCGAUDIO, &audio) < 0)
	    perror("VDeviceV4L::ioctl VIDIOCGAUDIO");

	audio.volume = 65535;
	audio.bass = 65535;
	audio.treble = 65535;
	if(muted)
		audio.flags |= VIDEO_AUDIO_MUTE | VIDEO_AUDIO_VOLUME;
	else
		audio.flags &= ~VIDEO_AUDIO_MUTE;

    if(ioctl(input_fd, VIDIOCSAUDIO, &audio) < 0)
		perror("VDeviceV4L::ioctl VIDIOCSAUDIO");
	return 0;
}


int VDeviceV4L::set_cloexec_flag(int desc, int value)
{
	int oldflags = fcntl(desc, F_GETFD, 0);
	if(oldflags < 0) return oldflags;
	if(value != 0) 
		oldflags |= FD_CLOEXEC;
	else
		oldflags &= ~FD_CLOEXEC;
	return fcntl(desc, F_SETFD, oldflags);
}





int VDeviceV4L::get_best_colormodel(Asset *asset)
{
	int result = BC_RGB888;

// Get best colormodel for hardware acceleration

	result = File::get_best_colormodel(asset, device->in_config->driver);


// Need to get color model before opening device but don't call this
// unless you want to open the device either.
	if(!initialization_complete)
	{
		device_colormodel = translate_colormodel(result);
		this->colormodel = result;
		switch(derivative)
		{
			case VIDEO4LINUX:
				v4l_init();
				break;
			case VIDEO4LINUX2:
				v4l2_init();
				break;
		};
		initialization_complete = 1;
	}
// printf("VDeviceV4L::get_best_colormodel %c%c%c%c\n", 
// 	((char*)&device_colormodel)[0],
// 	((char*)&device_colormodel)[1],
// 	((char*)&device_colormodel)[2],
// 	((char*)&device_colormodel)[3]);
	return result;
}

unsigned long VDeviceV4L::translate_colormodel(int colormodel)
{
	unsigned long result = 0;
	switch(derivative)
	{
		case VIDEO4LINUX:
			switch(colormodel)
			{
				case BC_YUV422:      result = VIDEO_PALETTE_YUV422;      break;
				case BC_YUV420P:     result = VIDEO_PALETTE_YUV420P;     break;
				case BC_YUV422P:     result = VIDEO_PALETTE_YUV422P;     break;
				case BC_YUV411P:     result = VIDEO_PALETTE_YUV411P;     break;
				case BC_RGB888:      result = VIDEO_PALETTE_RGB24;       break;
				default: result = VIDEO_PALETTE_RGB24; break;
			}
			break;

		case VIDEO4LINUX2:
			switch(colormodel)
			{
				case BC_YUV422:      result = V4L2_PIX_FMT_YUYV;        break;
				case BC_YUV420P:     result = V4L2_PIX_FMT_YUV420;      break;
				case BC_YUV422P:     result = V4L2_PIX_FMT_YVU422P;     break;
				case BC_YUV411P:     result = V4L2_PIX_FMT_YVU411P;     break;
				case BC_RGB888:      result = V4L2_PIX_FMT_RGB24;       break;
				default: result = V4L2_PIX_FMT_RGB24; break;
			}
			break;
	}
//printf("VDeviceV4L::translate_colormodel %d\n", result);
	return result;
}

int VDeviceV4L::set_channel(Channel *channel)
{
	return v4l1_set_channel(channel);
}

int VDeviceV4L::v4l1_set_channel(Channel *channel)
{
	struct video_channel channel_struct;
	struct video_tuner tuner_struct;
	unsigned long new_freq;

// Mute changed the input to TV
//	set_mute(1);

//printf("VDeviceV4L::v4l1_set_channel 1 %d\n", channel->input);
// Read norm/input defaults
	channel_struct.channel = channel->input;
	if(ioctl(input_fd, VIDIOCGCHAN, &channel_struct) < 0)
		perror("VDeviceV4L::v4l1_set_channel VIDIOCGCHAN");

// Set norm/input
	channel_struct.channel = channel->input;
	channel_struct.norm = v4l1_get_norm(channel->norm);
	if(ioctl(input_fd, VIDIOCSCHAN, &channel_struct) < 0)
		perror("VDeviceV4L::v4l1_set_channel VIDIOCSCHAN");

	if(channel_struct.flags & VIDEO_VC_TUNER)
	{
// Read tuner defaults
		tuner_struct.tuner = channel->input;
		if(ioctl(input_fd, VIDIOCGTUNER, &tuner_struct) < 0)
			perror("VDeviceV4L::v4l1_set_channel VIDIOCGTUNER");

// Set tuner
		tuner_struct.mode = v4l1_get_norm(channel->norm);
		if(ioctl(input_fd, VIDIOCSTUNER, &tuner_struct) < 0)
			perror("VDeviceV4L::v4l1_set_channel VIDIOCSTUNER");

		new_freq = chanlists[channel->freqtable].list[channel->entry].freq;
		new_freq = (int)(new_freq * 0.016);
		new_freq += channel->fine_tune;

		if(ioctl(input_fd, VIDIOCSFREQ, &new_freq) < 0)
			perror("VDeviceV4L::v4l1_set_channel VIDIOCSFREQ");
	}
//	set_mute(0);
	return 0;
}

int VDeviceV4L::v4l1_get_norm(int norm)
{
	switch(norm)
	{
		case NTSC:         return VIDEO_MODE_NTSC;         break;
		case PAL:          return VIDEO_MODE_PAL;          break;
		case SECAM:        return VIDEO_MODE_SECAM;        break;
	}
	return 0;
}

int VDeviceV4L::set_picture(int brightness, 
	int hue, 
	int color, 
	int contrast, 
	int whiteness)
{
	return v4l1_set_picture(brightness, 
		hue, 
		color, 
		contrast, 
		whiteness);
}


int VDeviceV4L::v4l1_set_picture(int brightness, 
	int hue, 
	int color, 
	int contrast, 
	int whiteness)
{
	brightness = (int)((float)brightness / 100 * 32767 + 32768);
	hue = (int)((float)hue / 100 * 32767 + 32768);
	color = (int)((float)color / 100 * 32767 + 32768);
	contrast = (int)((float)contrast / 100 * 32767 + 32768);
	whiteness = (int)((float)whiteness / 100 * 32767 + 32768);
	if(ioctl(input_fd, VIDIOCGPICT, &picture_params) < 0)
		perror("VDeviceV4L::v4l1_set_picture VIDIOCGPICT");
	picture_params.brightness = brightness;
	picture_params.hue = hue;
	picture_params.colour = color;
	picture_params.contrast = contrast;
	picture_params.whiteness = whiteness;
// Bogus.  Values are only set in the capture routine.
	picture_params.depth = 3;
	picture_params.palette = device_colormodel;
	if(ioctl(input_fd, VIDIOCSPICT, &picture_params) < 0)
		perror("VDeviceV4L::v4l1_set_picture VIDIOCSPICT");
	if(ioctl(input_fd, VIDIOCGPICT, &picture_params) < 0)
		perror("VDeviceV4L::v4l1_set_picture VIDIOCGPICT");
	return 0;
}


int VDeviceV4L::capture_frame(int capture_frame_number)
{
	switch(derivative)
	{
		case VIDEO4LINUX:
			struct video_mmap params;
			params.frame = capture_frame_number;
			params.width = device->in_config->w;
			params.height = device->in_config->h;
// Required to actually set the palette.
			params.format = device_colormodel;
// Tells the driver the buffer is available for writing
			if(ioctl(input_fd, VIDIOCMCAPTURE, &params) < 0)
				perror("VDeviceV4L::capture_frame VIDIOCMCAPTURE");
			break;
		case VIDEO4LINUX2:
			if(ioctl(input_fd, VIDIOC_QBUF, &v4l2_buffer_list[capture_frame_number]) < 0)
				perror("VideoDevice::capture_frame VIDIOC_QBUF");
			break;
	}
	return 0;
}

int VDeviceV4L::wait_v4l_frame()
{
//printf("VDeviceV4L::wait_v4l_frame 1 %d\n", capture_frame_number);
	if(ioctl(input_fd, VIDIOCSYNC, &capture_frame_number))
		perror("VDeviceV4L::wait_v4l_frame VIDIOCSYNC");
//printf("VDeviceV4L::wait_v4l_frame 2 %d\n", capture_frame_number);
	return 0;
}

int VDeviceV4L::wait_v4l2_frame()
{
	fd_set rdset;
	struct timeval timeout;
	FD_ZERO(&rdset);
	FD_SET(input_fd, &rdset);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if(select(input_fd + 1, &rdset, NULL, NULL, &timeout) <= 0)
		perror("VDeviceV4L::wait_v4l2_frame select");

	struct v4l2_buffer tempbuf;
	tempbuf.type = v4l2_buffer_list[0].vidbuf.type;
	if(ioctl(input_fd, VIDIOC_DQBUF, &tempbuf))
		perror("VDeviceV4L::wait_v4l2_frame VIDIOC_DQBUF");
	return 0;
}

int VDeviceV4L::read_v4l_frame(VFrame *frame)
{
	frame_to_vframe(frame, (unsigned char*)capture_buffer + capture_params.offsets[capture_frame_number]);
	return 0;
}

int VDeviceV4L::read_v4l2_frame(VFrame *frame)
{
	if(v4l2_buffer_list && (long)v4l2_buffer_list[capture_frame_number].data != -1)
		frame_to_vframe(frame, (unsigned char*)v4l2_buffer_list[capture_frame_number].data);
	return 0;
}

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

int VDeviceV4L::frame_to_vframe(VFrame *frame, unsigned char *input)
{
	int inwidth, inheight;
	int width, height;

	switch(derivative)
	{
		case VIDEO4LINUX:
			inwidth = window_params.width;
			inheight = window_params.height;
			break;
		case VIDEO4LINUX2:
			inwidth = v4l2_params.fmt.pix.width;
			inheight = v4l2_params.fmt.pix.height;
			break;
	}

	width = MIN(inwidth, frame->get_w());
	height = MIN(inheight, frame->get_h());
//printf("VDeviceV4L::frame_to_vframe %d %d\n", colormodel, frame->get_color_model());

	if(frame->get_color_model() == colormodel)
	{
		switch(frame->get_color_model())
		{
			case BC_RGB888:
			{
				unsigned char *row_in;
				unsigned char *row_out_start, *row_out_end;
				int bytes_per_inrow = inwidth * 3;
				int bytes_per_outrow = frame->get_bytes_per_line();
				unsigned char **rows_out = frame->get_rows();

				for(int i = 0; i < frame->get_h(); i++)
				{
					row_in = input + bytes_per_inrow * i;
					row_out_start = rows_out[i];
					row_out_end = row_out_start + 
						MIN(bytes_per_outrow, bytes_per_inrow);

					while(row_out_start < row_out_end)
					{
						*row_out_start++ = row_in[2];
						*row_out_start++ = row_in[1];
						*row_out_start++ = row_in[0];
						row_in += 3;
					}
				}
				break;
			}

			case BC_YUV420P:
			case BC_YUV411P:
				memcpy(frame->get_y(), input, width * height);
				memcpy(frame->get_u(), input + width * height, width * height / 4);
				memcpy(frame->get_v(), input + width * height + width * height / 4, width * height / 4);
				break;

			case BC_YUV422P:
				memcpy(frame->get_y(), input, width * height);
				memcpy(frame->get_u(), input + width * height, width * height / 2);
				memcpy(frame->get_v(), input + width * height + width * height / 2, width * height / 2);
				break;

			case BC_YUV422:
				memcpy(frame->get_data(), 
					input, 
					VFrame::calculate_data_size(width, 
						height, 
						-1, 
						frame->get_color_model()));
				break;
		}
	}
	else
	{
		VFrame *in_frame = new VFrame(input, 
			inwidth, 
			inheight, 
			colormodel, 
			-1);
		cmodel_transfer(frame->get_rows(), 
			in_frame->get_rows(),
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			in_frame->get_y(),
			in_frame->get_u(),
			in_frame->get_v(),
			0, 
			0, 
			inwidth, 
			inheight,
			0, 
			0, 
			frame->get_w(), 
			frame->get_h(),
			colormodel, 
			frame->get_color_model(),
			0,
			inwidth,
			inheight);
	}
	return 0;
}



int VDeviceV4L::next_frame(int previous_frame)
{
	int result = previous_frame + 1;

	switch(derivative)
	{
		case VIDEO4LINUX:
			if(result >= MIN(capture_params.frames, device->in_config->capture_length)) result = 0;
			break;
		case VIDEO4LINUX2:
			if(result >= v4l2_buffers.count) result = 0;
			break;
	};
	return result;
}

int VDeviceV4L::read_buffer(VFrame *frame)
{
	int result = 0;

	if(shared_memory)
	{
//printf("VDeviceV4L::read_buffer 1\n");
// Read the current frame
		switch(derivative)
		{
			case VIDEO4LINUX:
				if(!got_first_frame) v4l1_start_capture();
//printf("VDeviceV4L::read_buffer 2\n");
				wait_v4l_frame();
//printf("VDeviceV4L::read_buffer 3\n");
				read_v4l_frame(frame);
//printf("VDeviceV4L::read_buffer 4\n");
				break;
			case VIDEO4LINUX2:
				if(!got_first_frame) v4l2_start_capture();
				wait_v4l2_frame();
				read_v4l2_frame(frame);
				break;
		}
//printf("VDeviceV4L::read_buffer 5\n");

// Free this frame up for capturing
		capture_frame(capture_frame_number);
//printf("VDeviceV4L::read_buffer 6\n");
// Advance the frame to capture.
		capture_frame_number = next_frame(capture_frame_number);
//printf("VDeviceV4L::read_buffer 7\n");
	}
	else
	{
		switch(derivative)
		{
			case VIDEO4LINUX:
				read(input_fd, capture_buffer, capture_params.size);
				break;
			case VIDEO4LINUX2:
				read(input_fd, capture_buffer, v4l2_params.fmt.pix.sizeimage);
				break;
		}
	}
	got_first_frame = 1;

	return 0;
}
