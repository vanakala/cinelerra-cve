#include "assets.h"
#include "audioconfig.h"
#include "audiodevice.h"
#include "clip.h"
#include "file.inc"
#include "preferences.h"
#include "recordconfig.h"
#include "vdevice1394.h"
#include "vframe.h"
#include "playbackconfig.h"
#include "videodevice.h"

#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#ifdef HAVE_FIREWIRE





#include "avc1394.h"
#include "avc1394_vcr.h"
#include "rom1394.h"





#define CIP_N_NTSC 2436
#define CIP_D_NTSC 38400
#define CIP_N_PAL 1
#define CIP_D_PAL 16

VDevice1394::VDevice1394(VideoDevice *device)
 : VDeviceBase(device)
{
	initialize();
}

VDevice1394::~VDevice1394()
{
	close_all();
}

int VDevice1394::initialize()
{
	output_fd = 0;
	avc_handle = 0;
	grabber = 0;
	shared_frames = 0;
	user_frame = 0;
	temp_frame = 0;
	temp_frame2 = 0;
	encoder = 0;
}

int VDevice1394::open_input()
{
// Initialize sharing
	if(device->adevice &&
		device->adevice->in_config->driver == AUDIO_1394)
	{
		device->sharing = 1;
		shared_output_number = shared_input_number = 0;
		shared_ready = new int[device->in_config->capture_length];

		shared_output_lock = new Mutex[device->in_config->capture_length];
		shared_frames = new VFrame*[device->in_config->capture_length];
		for(int i = 0; i < device->in_config->capture_length; i++)
		{
			shared_frames[i] = new VFrame;
			shared_output_lock[i].lock();
			shared_ready[i] = 1;
		}
	}
	else
// Initialize grabbing
	{
		grabber = dv_grabber_new();
		if(dv_start_grabbing(grabber, 
			device->in_config->vfirewire_in_port, 
			device->in_config->vfirewire_in_channel, 
			device->in_config->capture_length))
		{
			dv_grabber_delete(grabber);
			grabber = 0;
			return 1;
		}
	}
	return 0;
}

int VDevice1394::open_output()
{
//printf("VDevice1394::open_output 1\n");
    output_fd = open("/dev/video1394", O_RDWR);
//printf("VDevice1394::open_output 2\n");

	if(output_fd <= 0)
	{
		perror("VDevice1394::open_output /dev/video1394");
		return 1;
	}
	else
	{
        avc_handle = raw1394_new_handle();
		if(avc_handle == 0)
			printf("VDevice1394::open_output avc_handle == 0\n");

		struct raw1394_portinfo pinf[16];
		int numcards = raw1394_get_port_info(avc_handle, pinf, 16);
		if(numcards < 0)
			printf("VDevice1394::open_output raw1394_get_port_info failed\n");
		if(raw1394_set_port(avc_handle, device->out_config->firewire_port) < 0)
			printf("VDevice1394::open_output raw1394_set_port\n");

    	int nodecount = raw1394_get_nodecount(avc_handle);
		int itemcount = 0;
  	  	rom1394_directory rom1394_dir;

		avc_id = -1;
    	for(int currentnode = 0; currentnode < nodecount; currentnode++) 
		{
        	rom1394_get_directory(avc_handle, currentnode, &rom1394_dir);
 
        	if((rom1394_get_node_type(&rom1394_dir) == ROM1394_NODE_TYPE_AVC) &&
            	avc1394_check_subunit_type(avc_handle, currentnode, AVC1394_SUBUNIT_TYPE_VCR)) 
			{
            	itemcount++;
// default the phyID to the first AVC node found
            	if(itemcount == 1) avc_id = currentnode;
				break;
        	}

        	rom1394_free_directory(&rom1394_dir);
    	}

		if(avc_id < 0)
			printf("VDevice1394::open_output no avc_id found.\n");

//printf("VDevice1394::open_output 1 %p %d\n", avc_handle, avc_id);
//		avc1394_vcr_record(avc_handle, avc_id);
//printf("VDevice1394::open_output 2\n");




        output_mmap.channel = device->out_config->firewire_channel;
        output_queue.channel = output_mmap.channel;
        output_mmap.sync_tag = 0;
        output_mmap.nb_buffers = 10;
        output_mmap.buf_size = 320 * 512;
        output_mmap.packet_size = 512;
        output_mmap.syt_offset = 19000;
        output_mmap.flags = VIDEO1394_VARIABLE_PACKET_SIZE;
    
        if(ioctl(output_fd, VIDEO1394_TALK_CHANNEL, &output_mmap) < 0)
		{
            perror("VDevice1394::open_output VIDEO1394_TALK_CHANNEL:");
        }
    
        output_buffer = (unsigned char*)mmap(0, 
			output_mmap.nb_buffers * output_mmap.buf_size,
            PROT_READ | PROT_WRITE, 
			MAP_SHARED, 
			output_fd, 
			0);
        if(output_buffer <= 0) 
		{
            perror("VDevice1394::open_output mmap");
        }
    
		unused_buffers = output_mmap.nb_buffers;
        output_queue.buffer = 0;
        output_queue.packet_sizes = packet_sizes;
        continuity_counter = 0;
        cip_counter = 0;

    }
//printf("VDevice1394::open_output 3\n");
	return 0;
}

int VDevice1394::interrupt_crash()
{
	if(device->sharing)
	{
		device->adevice->interrupt_crash();
	}
	else
	if(grabber && dv_grabber_crashed(grabber))
		dv_interrupt_grabber(grabber);
}

int VDevice1394::close_all()
{
//printf("VDevice1394::close_all 1 %d\n", device->sharing);
	if(device->sharing)
	{
//printf("VDevice1394::close_all 2\n");
		for(int i = 0; i < device->in_config->capture_length; i++)
		{
			delete shared_frames[i];
		}
//printf("VDevice1394::close_all 3\n");
		delete [] shared_frames;
//printf("VDevice1394::close_all 4\n");
		delete [] shared_output_lock;
//printf("VDevice1394::close_all 5\n");
		delete [] shared_ready;
//printf("VDevice1394::close_all 6\n");
		device->sharing = 0;
	}
	else
	if(grabber)
	{
// Interrupt the video device
		if(device->get_failed())
			dv_interrupt_grabber(grabber);

		dv_stop_grabbing(grabber);
		dv_grabber_delete(grabber);
	}

	if(output_fd)
	{
        output_queue.buffer = (output_mmap.nb_buffers + output_queue.buffer - 1) % output_mmap.nb_buffers;

        if(ioctl(output_fd, VIDEO1394_TALK_WAIT_BUFFER, &output_queue) < 0) 
		{
            fprintf(stderr, 
				"VDevice1394::close_all: VIDEO1394_TALK_WAIT_BUFFER %s: %s",
				output_queue,
				strerror(errno));
        }
        munmap(output_buffer, output_mmap.nb_buffers * output_mmap.buf_size);

        if(ioctl(output_fd, VIDEO1394_UNTALK_CHANNEL, &output_mmap.channel) < 0)
		{
            perror("VDevice1394::close_all: VIDEO1394_UNTALK_CHANNEL");
        }

        close(output_fd);
		
		if(avc_handle)
			raw1394_destroy_handle(avc_handle);
	}

	if(user_frame) delete user_frame;
	if(temp_frame) delete temp_frame;
	if(temp_frame2) delete temp_frame2;
	if(encoder) dv_delete(encoder);
//printf("VDevice1394::close_all 7\n");
	initialize();
//printf("VDevice1394::close_all 8\n");
	return 0;
}


int VDevice1394::get_shared_data(unsigned char *data, long size)
{
	if(device->sharing)
		if(shared_ready[shared_input_number])
		{
			shared_ready[shared_input_number] = 0;
			if(size && data)
			{
				shared_frames[shared_input_number]->allocate_compressed_data(size);
				memcpy(shared_frames[shared_input_number]->get_data(), data, size);
				shared_frames[shared_input_number]->set_compressed_size(size);
			}

			shared_output_lock[shared_input_number].unlock();
			shared_input_number++;
			if(shared_input_number >= device->in_config->capture_length) 
				shared_input_number = 0;
		}
	return 0;
}

int VDevice1394::stop_sharing()
{
	for(int i = 0; i < device->in_config->capture_length; i++)
		shared_output_lock[i].unlock();
	return 0;
}

int VDevice1394::read_buffer(VFrame *frame)
{
	unsigned char *data;
	long size = 0;
	int result = 0;

// Get from audio device
	if(device->sharing)
	{
		device->sharing_lock.lock();
		shared_output_lock[shared_output_number].lock();
		if(!device->done_sharing)
		{
			size = shared_frames[shared_output_number]->get_compressed_size();
			data = shared_frames[shared_output_number]->get_data();

			if(data && size)
			{
				frame->allocate_compressed_data(size);
				memcpy(frame->get_data(), data, size);
				frame->set_compressed_size(size);
			}
		}
		else
		{
			size = 0;
			data = 0;
		}

		device->sharing_lock.unlock();

		shared_ready[shared_output_number] = 1;
		shared_output_number++;
		if(shared_output_number >= device->in_config->capture_length) shared_output_number = 0;
	}
	else
// Get from libdv directly
	if(grabber)
	{
//printf("VDevice1394::read_buffer 1\n");
		result = dv_grab_frame(grabber, &data, &size);

		if(!result)
		{
			frame->allocate_compressed_data(size);
			memcpy(frame->get_data(), data, size);
			frame->set_compressed_size(size);
		}
printf("VDevice1394::read_buffer 1\n");
		dv_unlock_frame(grabber);
printf("VDevice1394::read_buffer 2\n");
	}

	return result;
}


void VDevice1394::new_output_buffer(VFrame **outputs,
	int colormodel)
{
//printf("VDevice1394::new_output_buffer 1 %d\n", colormodel);
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

int VDevice1394::write_buffer(VFrame **frame, EDL *edl)
{
	VFrame *ptr = 0;
	VFrame *input = frame[0];

//printf("VDevice1394::write_buffer 1\n");
	if(input->get_color_model() != BC_COMPRESSED)
	{
//printf("VDevice1394::write_buffer 2\n");
		if(!temp_frame) temp_frame = new VFrame;
		if(!encoder) encoder = dv_new();
		ptr = temp_frame;

// Exact resolution match
		if(input->get_color_model() == BC_YUV422 &&
			input->get_w() == 720 &&
			(input->get_h() == 480 ||
			input->get_h() == 576))
		{
//printf("VDevice1394::write_buffer 3\n");
			int norm = (input->get_h() == 480) ? DV_NTSC : DV_PAL;
			int data_size = (norm == DV_NTSC) ? DV_NTSC_SIZE : DV_PAL_SIZE;
			temp_frame->allocate_compressed_data(data_size);
			temp_frame->set_compressed_size(data_size);

			dv_write_video(encoder,
				temp_frame->get_data(),
				input->get_rows(),
				BC_YUV422,
				norm);
			ptr = temp_frame;
//printf("VDevice1394::write_buffer 4\n");
		}
		else
// Convert resolution and color model before compressing
		{
//printf("VDevice1394::write_buffer 5\n");
			if(!temp_frame2)
			{
				int h = input->get_h();
				if(h != 480 && h != 576) h = 480;

				temp_frame2 = new VFrame(0,
					720,
					h,
					BC_YUV422);
				
			}
//printf("VDevice1394::write_buffer 5\n");

			int norm = (input->get_h() == 480) ? DV_NTSC : DV_PAL;
			int data_size = (norm == DV_NTSC) ? DV_NTSC_SIZE : DV_PAL_SIZE;
			temp_frame->allocate_compressed_data(data_size);
			temp_frame->set_compressed_size(data_size);

//printf("VDevice1394::write_buffer 5 %p %p %d %d\n",
// 				temp_frame2,
// 				temp_frame2->get_rows(), /* Leave NULL if non existent */
// 				input->get_rows(),
// 				input->get_bytes_per_line(),       /* For planar use the luma rowspan */
// 				temp_frame2->get_bytes_per_line());


			cmodel_transfer(temp_frame2->get_rows(), /* Leave NULL if non existent */
				input->get_rows(),
				temp_frame2->get_y(), /* Leave NULL if non existent */
				temp_frame2->get_u(),
				temp_frame2->get_v(),
				input->get_y(), /* Leave NULL if non existent */
				input->get_u(),
				input->get_v(),
				0,        /* Dimensions to capture from input frame */
				0, 
				MIN(temp_frame2->get_w(), input->get_w()),
				MIN(temp_frame2->get_h(), input->get_h()),
				0,       /* Dimensions to project on output frame */
				0, 
				MIN(temp_frame2->get_w(), input->get_w()),
				MIN(temp_frame2->get_h(), input->get_h()),
				input->get_color_model(), 
				BC_YUV422,
				0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
				input->get_bytes_per_line(),       /* For planar use the luma rowspan */
				temp_frame2->get_bytes_per_line());     /* For planar use the luma rowspan */

//printf("VDevice1394::write_buffer 5\n");
			dv_write_video(encoder,
				temp_frame->get_data(),
				temp_frame2->get_rows(),
				BC_YUV422,
				norm);



//printf("VDevice1394::write_buffer 5\n");

			ptr = temp_frame;
//printf("VDevice1394::write_buffer 6\n");
		}
	}
	else
		ptr = input;





//printf("VDevice1394::write_buffer 7\n");



	while(unused_buffers--)
	{
//printf("VDevice1394::write_buffer 8\n");
		int is_pal = (ptr->get_h() == 576);
		unsigned char *output = output_buffer + output_queue.buffer * output_mmap.buf_size;
		int output_size = 320;
		int packets_per_frame = (is_pal ? 300 : 250);
		int min_packet_size = output_mmap.packet_size;
		long frame_size = packets_per_frame * 480;
		char vdata = 0;
		unsigned char *data = ptr->get_data();
		unsigned int *packet_sizes = this->packet_sizes;
//printf("VDevice1394::write_buffer 9\n");

    	if(cip_counter == 0) 
		{
        	if(packets_per_frame == 250) 
			{
            	cip_n = CIP_N_NTSC;
            	cip_d = CIP_D_NTSC;
            	f50_60 = 0x00;
        	}
			else 
			{
            	cip_n = CIP_N_PAL;
            	cip_d = CIP_D_PAL;
            	f50_60 = 0x80;
        	}
        	cip_counter = cip_n;
    	}
//printf("VDevice1394::write_buffer 10\n");

		for(int i = 0; i < output_size && vdata < frame_size; i++)
		{
        	unsigned char *p = output;
        	int want_sync = (cip_counter > cip_d);

/* Source node ID ! */
       	 	*p++ = 0x01; 
/* Packet size in quadlets (480 / 4) - this stays the same even for empty packets */
      	  	*p++ = 0x78; 
      	  	*p++ = 0x00;
     	   	*p++ = continuity_counter;

/* const */
        	*p++ = 0x80; 
/* high bit = 50/60 indicator */
        	*p++ = f50_60; 

/* timestamp - generated in driver */
        	*p++ = 0xff; 
/* timestamp */
        	*p++ = 0xff; 

/* video data */
        	if(!want_sync)
			{
            	continuity_counter++;
            	cip_counter += cip_n;

            	memcpy(p, data + vdata, 480);
            	p += 480;
            	vdata += 480;
        	}
        	else
            	cip_counter -= cip_d;

        	*packet_sizes++ = p - output;
        	output += min_packet_size;
		}
   		*packet_sizes++ = 0;

// printf("VDevice1394::write_buffer 12 %02x %02x %02x %02x %02x %02x %02x %02x\n",
// output[0],output[1], output[2], output[3], output[4], output[5], output[6], output[7]);

		if(ioctl(output_fd, VIDEO1394_TALK_QUEUE_BUFFER, &output_queue) < 0)
		{
        	perror("VDevice1394::write_buffer VIDEO1394_TALK_QUEUE_BUFFER");
    	}
//printf("VDevice1394::write_buffer 13\n");

    	output_queue.buffer++;
		if(output_queue.buffer >= output_mmap.nb_buffers) output_queue.buffer = 0;
	}
//printf("VDevice1394::write_buffer 14\n");

    if (ioctl(output_fd, VIDEO1394_TALK_WAIT_BUFFER, &output_queue) < 0) 
	{
        perror("VDevice1394::write_buffer VIDEO1394_TALK_WAIT_BUFFER");
    }
//printf("VDevice1394::write_buffer 15\n");

    unused_buffers = 1;


	return 0;
}

int VDevice1394::can_copy_from(Asset *asset, int output_w, int output_h)
{
	return 0;
}











#endif // HAVE_FIREWIRE
