#undef _FILE_OFFSET_BITS
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE

#include "assets.h"
#include "channel.h"
#include "chantables.h"
#include "clip.h"
#include "condition.h"
#include "file.h"
#include "picture.h"
#include "preferences.h"
#include "quicktime.h"
#include "recordconfig.h"
#include "vdevicev4l2.h"
#include "vframe.h"
#include "videodevice.h"

#ifdef HAVE_V4L2

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>








VDeviceV4L2Thread::VDeviceV4L2Thread(VideoDevice *device, int color_model)
 : Thread(1, 0, 0)
{
	this->device = device;
	this->color_model = color_model;

	video_lock = new Condition(0, "VDeviceV4L2Thread::video_lock");
	buffer_lock = new Mutex("VDeviceV4L2Thread::buffer_lock");
	device_buffers = 0;
	buffer_valid = 0;
	current_inbuffer = 0;
	current_outbuffer = 0;
	total_buffers = 0;
	first_frame = 1;
	done = 0;
	input_fd = 0;
	total_valid = 0;
}

VDeviceV4L2Thread::~VDeviceV4L2Thread()
{
	if(Thread::running())
	{
		done = 1;
		Thread::cancel();
		Thread::join();
	}

	if(buffer_valid)
	{
		delete [] buffer_valid;
	}

// Buffers are not unmapped by close.
	if(device_buffers)
	{
		for(int i = 0; i < total_buffers; i++)
		{
			if(color_model == BC_COMPRESSED)
			{
				if(device_buffers[i]->get_data()) 
					munmap(device_buffers[i]->get_data(),
						device_buffers[i]->get_compressed_allocated());
			}
			else
			{
				if(device_buffers[i]->get_data()) 
					munmap(device_buffers[i]->get_data(),
						device_buffers[i]->get_data_size());
			}
			delete device_buffers[i];
		}
		delete [] device_buffers;
	}

	delete video_lock;
	delete buffer_lock;

	if(input_fd > 0) close(input_fd);
}

void VDeviceV4L2Thread::start()
{
// Allocate user space buffers
	total_buffers = device->in_config->capture_length;
	total_buffers = MAX(total_buffers, 2);
	device_buffers = new VFrame*[total_buffers];
	buffer_valid = new int[total_buffers];
	for(int i = 0; i < total_buffers; i++)
	{
		device_buffers[i] = new VFrame;
	}
	bzero(buffer_valid, sizeof(int) * total_buffers);

	Thread::start();
}



void VDeviceV4L2Thread::run()
{
// Set up the device
	int error = 0;
	Thread::enable_cancel();



	if((input_fd = open(device->in_config->v4l2jpeg_in_device, 
		O_RDWR)) < 0)
	{
		perror("VDeviceV4L2Thread::run");
		error = 1;
	}

	if(!error)
	{
		device->set_cloexec_flag(input_fd, 1);


		struct v4l2_capability cap;
		if(ioctl(input_fd, VIDIOC_QUERYCAP, &cap))
			perror("VDeviceV4L2Thread::run VIDIOC_QUERYCAP");

// printf("VDeviceV4L2Thread::run input_fd=%d driver=%s card=%s bus_info=%s version=%d\n",
// input_fd,
// cap.driver,
// cap.card,
// cap.bus_info,
// cap.version);
// printf("    %s%s%s%s%s%s%s%s%s%s%s%s\n", 
// (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ? "V4L2_CAP_VIDEO_CAPTURE " : "",
// (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) ? "V4L2_CAP_VIDEO_OUTPUT " : "",
// (cap.capabilities & V4L2_CAP_VIDEO_OVERLAY) ? "V4L2_CAP_VIDEO_OVERLAY " : "",
// (cap.capabilities & V4L2_CAP_VBI_CAPTURE) ? "V4L2_CAP_VBI_CAPTURE " : "",
// (cap.capabilities & V4L2_CAP_VBI_OUTPUT) ? "V4L2_CAP_VBI_OUTPUT " : "",
// (cap.capabilities & V4L2_CAP_RDS_CAPTURE) ? "V4L2_CAP_RDS_CAPTURE " : "",
// (cap.capabilities & V4L2_CAP_TUNER) ? "V4L2_CAP_TUNER " : "",
// (cap.capabilities & V4L2_CAP_AUDIO) ? "V4L2_CAP_AUDIO " : "",
// (cap.capabilities & V4L2_CAP_RADIO) ? "V4L2_CAP_RADIO " : "",
// (cap.capabilities & V4L2_CAP_READWRITE) ? "V4L2_CAP_READWRITE " : "",
// (cap.capabilities & V4L2_CAP_ASYNCIO) ? "V4L2_CAP_ASYNCIO " : "",
// (cap.capabilities & V4L2_CAP_STREAMING) ? "V4L2_CAP_STREAMING " : "");


// Set up frame rate
		struct v4l2_streamparm v4l2_parm;
		v4l2_parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(ioctl(input_fd, VIDIOC_G_PARM, &v4l2_parm) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_G_PARM");
		if(v4l2_parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
		{
			v4l2_parm.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;


			v4l2_parm.parm.capture.timeperframe.numerator = 1;
			v4l2_parm.parm.capture.timeperframe.denominator = 
				(unsigned long)((float)1 / 
				device->frame_rate * 
				10000000);
			if(ioctl(input_fd, VIDIOC_S_PARM, &v4l2_parm) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_PARM");

			if(ioctl(input_fd, VIDIOC_G_PARM, &v4l2_parm) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_G_PARM");
		}

// Set up data format
		struct v4l2_format v4l2_params;
		v4l2_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(ioctl(input_fd, VIDIOC_G_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_G_FMT");
		v4l2_params.fmt.pix.width = device->in_config->w;
		v4l2_params.fmt.pix.height = device->in_config->h;

		if(color_model == BC_COMPRESSED)
			v4l2_params.fmt.pix.pixelformat = 
				V4L2_PIX_FMT_MJPEG;
		else
			v4l2_params.fmt.pix.pixelformat = 
				VDeviceV4L2::cmodel_to_device(color_model);


		if(ioctl(input_fd, VIDIOC_S_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_S_FMT");
		if(ioctl(input_fd, VIDIOC_G_FMT, &v4l2_params) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_G_FMT");

// Set input
		Channel *device_channel = 0;
		if(device->channel->input >= 0 &&
			device->channel->input < device->get_inputs()->total)
		{
			device_channel = device->get_inputs()->values[
				device->channel->input];
		}

// Try first input
		if(!device_channel)
		{
			if(device->get_inputs()->total)
			{
				device_channel = device->get_inputs()->values[0];
				printf("VDeviceV4L2Thread::run user channel not found.  Using %s\n",
					device_channel->device_name);
			}
			else
			{
				printf("VDeviceV4L2Thread::run channel \"%s\" not found.\n",
					device->channel->title);
			}
		}

// Translate input to API structures
		struct v4l2_tuner tuner;
		int input = 0;
		if(device_channel)
		{
			tuner.index = device_channel->device_index;
			input = device_channel->device_index;
		}
		else
		{
			tuner.index = 0;
			input = 0;
		}

		tuner.type = V4L2_TUNER_ANALOG_TV;


		if(ioctl(input_fd, VIDIOC_S_INPUT, &input) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_S_INPUT");

		if(cap.capabilities & V4L2_CAP_TUNER)
		{
			if(ioctl(input_fd, VIDIOC_S_TUNER, &tuner) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_TUNER");
// Set frequency
			struct v4l2_frequency frequency;
			frequency.tuner = device->channel->tuner;
			frequency.type = V4L2_TUNER_ANALOG_TV;
			frequency.frequency = chanlists[
				device->channel->freqtable].list[
					device->channel->entry].freq;
			if(ioctl(input_fd, VIDIOC_S_FREQUENCY, &frequency) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_FREQUENCY");
		}

// Set norm
		v4l2_std_id std_id;
		switch(device->channel->norm)
		{
			case NTSC: std_id = V4L2_STD_NTSC; break;
			case PAL: std_id = V4L2_STD_PAL; break;
			case SECAM: std_id = V4L2_STD_SECAM; break;
			default: std_id = V4L2_STD_NTSC_M; break;
		}

		if(ioctl(input_fd, VIDIOC_S_STD, &std_id))
			perror("VDeviceV4L2Thread::run VIDIOC_S_STD");


// printf("VDeviceV4L2Thread::run input=%d norm=%d\n",
// device->channel->input,
// device->channel->norm);

// Set picture controls
		Picture *picture = device->picture;
		for(int i = 0; i < picture->controls.total; i++)
		{
			struct v4l2_control ctrl_arg;
			PictureItem *item = picture->controls.values[i];
//printf("VDeviceV4L2Thread::run %x %d\n", item->device_id, item->value);
			ctrl_arg.id = item->device_id;
			ctrl_arg.value = item->value;
			if(ioctl(input_fd, VIDIOC_S_CTRL, &ctrl_arg) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_CTRL");
		}

// Set compression
		if(color_model == BC_COMPRESSED)
		{
			struct v4l2_jpegcompression jpeg_arg;
			if(ioctl(input_fd, VIDIOC_G_JPEGCOMP, &jpeg_arg) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_G_JPEGCOMP");
			jpeg_arg.quality = device->quality / 2;
			if(ioctl(input_fd, VIDIOC_S_JPEGCOMP, &jpeg_arg) < 0)
				perror("VDeviceV4L2Thread::run VIDIOC_S_JPEGCOMP");
		}

// Allocate buffers.  Errors here are fatal.
		Thread::disable_cancel();
		struct v4l2_requestbuffers requestbuffers;

		requestbuffers.count = total_buffers;
		requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		requestbuffers.memory = V4L2_MEMORY_MMAP;
		if(ioctl(input_fd, VIDIOC_REQBUFS, &requestbuffers) < 0)
		{
			perror("VDeviceV4L2Thread::run VIDIOC_REQBUFS");
			error = 1;
		}
		else
		{
			for(int i = 0; i < requestbuffers.count; i++)
			{
				struct v4l2_buffer buffer;
				buffer.type = requestbuffers.type;
				buffer.index = i;
				if(ioctl(input_fd, VIDIOC_QUERYBUF, &buffer) < 0)
				{
					perror("VDeviceV4L2Thread::run VIDIOC_QUERYBUF");
					error = 1;
					break;
				}

				unsigned char *data = (unsigned char*)mmap(NULL,
					buffer.length,
					PROT_READ | PROT_WRITE,
					MAP_SHARED,
					input_fd,
					buffer.m.offset);
				if(data == MAP_FAILED)
				{
					perror("VDeviceV4L2Thread::run mmap");
					error = 1;
					break;
				}

				VFrame *frame = device_buffers[i];
				if(color_model == BC_COMPRESSED)
				{
					frame->set_compressed_memory(data,
						0,
						buffer.length);
				}
				else
				{
					int y_offset = 0;
					int u_offset = 0;
					int v_offset = 0;

					switch(color_model)
					{
						case BC_YUV422P:
							u_offset = device->in_config->w * device->in_config->h;
							v_offset = device->in_config->w * device->in_config->h + device->in_config->w * device->in_config->h / 2;
							break;
						case BC_YUV420P:
						case BC_YUV411P:
							u_offset = device->in_config->w * device->in_config->h;
							v_offset = device->in_config->w * device->in_config->h + device->in_config->w * device->in_config->h / 4;
							break;
					}

					frame->set_memory(data,
						y_offset,
						u_offset,
						v_offset);
				}
			}
		}
		Thread::enable_cancel();
	}


// Start capturing
	if(!error)
	{
		for(int i = 0; i < total_buffers; i++)
		{
			struct v4l2_buffer buffer;
			buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buffer.index = i;
			if(ioctl(input_fd, VIDIOC_QBUF, &buffer) < 0)
			{
				perror("VDeviceV4L2Thread::run VIDIOC_QBUF");
				break;
			}
		}


		int streamon_arg = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(ioctl(input_fd, VIDIOC_STREAMON, &streamon_arg) < 0)
			perror("VDeviceV4L2Thread::run VIDIOC_STREAMON");
	}

	Thread::disable_cancel();


//printf("VDeviceV4L2Thread::run 1 %d\n", input_fd);
// Read buffers continuously
	first_frame = 0;
	while(!done && !error)
	{
		Thread::enable_cancel();
		struct v4l2_buffer buffer;
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

// The driver returns the first buffer not queued, so only one buffer
// can be unqueued at a time.
		if(ioctl(input_fd, VIDIOC_DQBUF, &buffer) < 0)
		{
			sleep(1);
			Thread::disable_cancel();
			perror("VDeviceV4L2Thread::run VIDIOC_DQBUF");
		}
		else
		{
			Thread::disable_cancel();
// printf("VDeviceV4L2Thread::run 10 index=%d bytesused=%d field=%d length=%d\n",
// buffer.index,
// buffer.bytesused,
// buffer.field,
// buffer.length);

			buffer_lock->lock("VDeviceV4L2Thread::run");

// Set output frame as valid and set data size
			current_inbuffer = buffer.index;
			if(color_model == BC_COMPRESSED)
			{
				device_buffers[current_inbuffer]->set_compressed_size(
					buffer.bytesused);
			}

			if(!buffer_valid[current_inbuffer])
			{
// Increase valid total only if current is invalid
				buffer_valid[current_inbuffer] = 1;
				total_valid++;
				buffer_lock->unlock();
				video_lock->unlock();
			}
			else
			{
// Driver won't block on the next QBUF call because we're not requeueing the buffer.
				buffer_lock->unlock();
				video_lock->unlock();
				usleep(33000);
			}
		}
	}
}

VFrame* VDeviceV4L2Thread::get_buffer(int *timed_out)
{
	VFrame *result = 0;
	*timed_out = 0;

// Acquire buffer table
	buffer_lock->lock("VDeviceV4L2Thread::read_frame 1");


// Test for buffer availability
	while(total_valid < 2 && !*timed_out && !first_frame)
	{
		buffer_lock->unlock();
		*timed_out = video_lock->timed_lock(BUFFER_TIMEOUT, 
			"VDeviceV4L2Thread::read_frame 2");
		buffer_lock->lock("VDeviceV4L2Thread::read_frame 3");
	}

// Copy frame
	if(total_valid >= 2)
	{
		result = device_buffers[current_outbuffer];
	}

	buffer_lock->unlock();

	return result;
}

void VDeviceV4L2Thread::put_buffer()
{
	buffer_lock->lock("VDeviceV4L2Thread::put_buffer");
	buffer_valid[current_outbuffer] = 0;


// Release buffer for capturing
	struct v4l2_buffer buffer;
	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.index = current_outbuffer;
	if(ioctl(input_fd, VIDIOC_QBUF, &buffer) < 0)
		perror("VDeviceV4L2Thread::put_buffer 1 VIDIOC_QBUF");

	current_outbuffer++;
	total_valid--;
	if(current_outbuffer >= total_buffers)
		current_outbuffer = 0;
	buffer_lock->unlock();
}


















VDeviceV4L2::VDeviceV4L2(VideoDevice *device)
 : VDeviceBase(device)
{
	initialize();
}

VDeviceV4L2::~VDeviceV4L2()
{
	close_all();
}

int VDeviceV4L2::close_all()
{
	if(thread) delete thread;
	thread = 0;
	return 0;
}


int VDeviceV4L2::initialize()
{
	thread = 0;
	return 0;
}



int VDeviceV4L2::open_input()
{
	int result = get_sources(device,
		device->in_config->v4l2_in_device);
	device->channel->use_frequency = 1;
	device->channel->use_fine = 1;
	return result;
}

int VDeviceV4L2::get_sources(VideoDevice *device,
	char *path)
{
	int input_fd = -1;

	device->channel->use_norm = 1;
	device->channel->use_input = 1;

	if((input_fd = open(path, O_RDWR)) < 0)
	{
		perror("VDeviceV4L::open_input");
		return 1;
	}
	else
	{
// Get the inputs
		int i = 0;
		int done = 0;
		char *new_source;

		while(!done && i < 20)
		{
			struct v4l2_input arg;
			bzero(&arg, sizeof(arg));
			arg.index = i;
			
			if(ioctl(input_fd, VIDIOC_ENUMINPUT, &arg) < 0)
			{
// Finished
				done = 1;
			}
			else
			{
				Channel *channel = device->new_input_source((char*)arg.name);
				channel->device_index = i;
				channel->tuner = arg.tuner;
			}
			i++;
		}

// Get the picture controls
		for(i = V4L2_CID_BASE; i < V4L2_CID_LASTP1; i++)
		{
			struct v4l2_queryctrl arg;
			bzero(&arg, sizeof(arg));
			arg.id = i;
// This returns errors for unsupported controls which is what we want.
			if(!ioctl(input_fd, VIDIOC_QUERYCTRL, &arg))
			{
				PictureItem *item = device->picture->new_item((char*)arg.name);
				item->device_id = arg.id;
				item->min = arg.minimum;
				item->max = arg.maximum;
				item->step = arg.step;
				item->default_ = arg.default_value;
				item->type = arg.type;
				item->value = arg.default_value;
			}
		}

		close(input_fd);
	}
	return 0;
}

int VDeviceV4L2::cmodel_to_device(int color_model)
{
	switch(color_model)
	{
		case BC_YUV422:
			return V4L2_PIX_FMT_YUYV;
			break;
		case BC_YUV411P:
			return V4L2_PIX_FMT_Y41P;
			break;
		case BC_YUV420P:
			return V4L2_PIX_FMT_YVU420;
			break;
		case BC_YUV422P:
			return V4L2_PIX_FMT_YUV422P;
			break;
		case BC_RGB888:
			return V4L2_PIX_FMT_RGB24;
			break;
	}
	return 0;
}

int VDeviceV4L2::get_best_colormodel(Asset *asset)
{
	int result = BC_RGB888;
	result = File::get_best_colormodel(asset, device->in_config->driver);
	return result;
}


int VDeviceV4L2::read_buffer(VFrame *frame)
{
	int result = 0;

	if((device->channel_changed || device->picture_changed) && thread)
	{
		delete thread;
		thread = 0;
	}

	if(!thread)
	{
		device->channel_changed = 0;
		device->picture_changed = 0;
		thread = new VDeviceV4L2Thread(device, frame->get_color_model());
		thread->start();
	}


// Get buffer from thread
	int timed_out;
	VFrame *buffer = thread->get_buffer(&timed_out);
	if(buffer)
	{
		frame->copy_from(buffer);
		thread->put_buffer();
	}
	else
	{
// Driver in 2.6.4 needs to be restarted when it loses sync.
		if(timed_out)
		{
			delete thread;
			thread = 0;
		}
		result = 1;
	}


	return result;
}

#endif
