#ifdef HAVE_FIREWIRE




#include "condition.h"
#include "device1394output.h"
#include "mutex.h"
#include "timer.h"
#include "vframe.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>


#include "avc1394.h"
#include "avc1394_vcr.h"
#include "rom1394.h"




// Crazy DV internals
#define CIP_N_NTSC 2436
#define CIP_D_NTSC 38400
#define CIP_N_PAL 1
#define CIP_D_PAL 16
#define OUTPUT_SAMPLES 262144
#define BUFFER_TIMEOUT 500000


Device1394Output::Device1394Output()
 : Thread(1, 0, 0)
{
	buffer = 0;
	buffer_size = 0;
	total_buffers = 0;
	current_inbuffer = 0;
	current_outbuffer = 0;
	done = 0;
	audio_lock = 0;
	video_lock = 0;
	start_lock = 0;
	buffer_lock = 0;
	position_lock = 0;
	video_encoder = 0;
	audio_encoder = 0;
	audio_buffer = 0;
	audio_samples = 0;
	output_fd = -1;
//	avc_handle = 0;
	temp_frame = 0;
	temp_frame2 = 0;
	audio_position = 0;
	interrupted = 0;
	have_video = 0;
}

Device1394Output::~Device1394Output()
{
	if(Thread::running())
	{
		done = 1;
		start_lock->unlock();
		Thread::cancel();
		Thread::join();
	}

	if(buffer)
	{
		for(int i = 0; i < total_buffers; i++)
		{
			if(buffer[i]) delete [] buffer[i];
		}
		delete [] buffer;
		delete [] buffer_size;
		delete [] buffer_valid;
	}

	if(audio_lock) delete audio_lock;
	if(video_lock) delete video_lock;
	if(start_lock) delete start_lock;
	if(audio_buffer) delete [] audio_buffer;

	if(output_fd >= 0)
	{
        output_queue.buffer = (output_mmap.nb_buffers + output_queue.buffer - 1) % output_mmap.nb_buffers;

        if(ioctl(output_fd, VIDEO1394_TALK_WAIT_BUFFER, &output_queue) < 0) 
		{
            fprintf(stderr, 
				"Device1394::close_all: VIDEO1394_TALK_WAIT_BUFFER %s: %s",
				output_queue,
				strerror(errno));
        }
        munmap(output_buffer, output_mmap.nb_buffers * output_mmap.buf_size);

        if(ioctl(output_fd, VIDEO1394_UNTALK_CHANNEL, &output_mmap.channel) < 0)
		{
            perror("Device1394::close_all: VIDEO1394_UNTALK_CHANNEL");
        }

        close(output_fd);

//		if(avc_handle)
//			raw1394_destroy_handle(avc_handle);
	}

	if(temp_frame) delete temp_frame;
	if(temp_frame2) delete temp_frame2;
	if(video_encoder) dv_delete(video_encoder);
	if(audio_encoder) dv_delete(audio_encoder);
	if(buffer_lock) delete buffer_lock;
	if(position_lock) delete position_lock;
}


int Device1394Output::open(char *path,
	int port,
	int channel,
	int length,
	int channels, 
	int bits, 
	int samplerate,
	int syt)
{
	this->channels = channels;
	this->bits = bits;
	this->samplerate = samplerate;
	this->total_buffers = length;
	this->syt = syt;

//printf("Device1394::open_output 2 %s %d %d %d %d\n", path, port, channel, length, syt);
	if(output_fd < 0)
	{
    	output_fd = ::open(path, O_RDWR);

		if(output_fd <= 0)
		{
			fprintf(stderr, 
				"Device1394Output::open path=%s: %s", 
				path,
				strerror(errno));
			return 1;
		}
		else
		{
//         	avc_handle = raw1394_new_handle();
// 			if(avc_handle == 0)
// 				printf("Device1394::open_output avc_handle == 0\n");
// 
// 			struct raw1394_portinfo pinf[16];
// 			int numcards = raw1394_get_port_info(avc_handle, pinf, 16);
// 			if(numcards < 0)
// 				printf("Device1394::open_output raw1394_get_port_info failed\n");
// 			if(raw1394_set_port(avc_handle, out_config->firewire_port) < 0)
// 				printf("Device1394::open_output raw1394_set_port\n");
// 
//     		int nodecount = raw1394_get_nodecount(avc_handle);
// 			int itemcount = 0;
//   	  		rom1394_directory rom1394_dir;
// 
// 			avc_id = -1;
//     		for(int currentnode = 0; currentnode < nodecount; currentnode++) 
// 			{
//         		rom1394_get_directory(avc_handle, currentnode, &rom1394_dir);
// 
//         		if((rom1394_get_node_type(&rom1394_dir) == ROM1394_NODE_TYPE_AVC) &&
//             		avc1394_check_subunit_type(avc_handle, currentnode, AVC1394_SUBUNIT_TYPE_VCR)) 
// 				{
//             		itemcount++;
// // default the phyID to the first AVC node found
//             		if(itemcount == 1) avc_id = currentnode;
// 					break;
//         		}
// 
//         		rom1394_free_directory(&rom1394_dir);
//     		}
// 
// 			if(avc_id < 0)
// 				printf("Device1394::open_output no avc_id found.\n");
// 
//printf("Device1394::open_output 1 %p %d\n", avc_handle, avc_id);
//		avc1394_vcr_record(avc_handle, avc_id);




        	output_mmap.channel = channel;
        	output_queue.channel = channel;
        	output_mmap.sync_tag = 0;
        	output_mmap.nb_buffers = total_buffers;
        	output_mmap.buf_size = 320 * 512;
        	output_mmap.packet_size = 512;
// Shouldn't this be handled by the video1394 driver?
// dvgrab originally used 19000
// JVC DVL300 -> 30000
        	output_mmap.syt_offset = syt;
        	output_mmap.flags = VIDEO1394_VARIABLE_PACKET_SIZE;


// printf("Device1394Output::open %d %d %d %d %d %d %d\n", 
// output_mmap.channel, output_queue.channel, output_mmap.sync_tag, output_mmap.nb_buffers, output_mmap.buf_size, output_mmap.packet_size, output_mmap.syt_offset, output_mmap.flags);
        	if(ioctl(output_fd, VIDEO1394_TALK_CHANNEL, &output_mmap) < 0)
			{
            	perror("Device1394Output::open VIDEO1394_TALK_CHANNEL:");
        	}

        	output_buffer = (unsigned char*)mmap(0, 
				output_mmap.nb_buffers * output_mmap.buf_size,
            	PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				output_fd, 
				0);
        	if(output_buffer <= 0)
			{
            	perror("Device1394Output::open mmap");
        	}

			unused_buffers = output_mmap.nb_buffers;
        	output_queue.buffer = 0;
        	output_queue.packet_sizes = packet_sizes;
        	continuity_counter = 0;
        	cip_counter = 0;

// Create buffers
			buffer = new char*[total_buffers];
			for(int i = 0; i < length; i++)
				buffer[i] = new char[DV_PAL_SIZE];
			buffer_size = new int[total_buffers];
			buffer_valid = new int[total_buffers];
			bzero(buffer_size, sizeof(int) * total_buffers);
			bzero(buffer_valid, sizeof(int) * total_buffers);
			bzero(buffer, sizeof(char*) * total_buffers);
			video_lock = new Condition(0);
			audio_lock = new Condition(0);
			start_lock = new Condition(0);
			buffer_lock = new Mutex;
			position_lock = new Mutex;
			encoder = dv_new();
			audio_buffer = new char[OUTPUT_SAMPLES * channels * bits / 8];
			Thread::start();
    	}
	}
	return 0;
}

void Device1394Output::run()
{
	Thread::enable_cancel();
	start_lock->lock();
	Thread::disable_cancel();

//Timer timer;
// Write buffers continuously
	while(!done)
	{
// Get current buffer to play
		if(done) return;
//timer.update();

//printf("Device1394Output::run 1\n");
		buffer_lock->lock();
//printf("Device1394Output::run 10\n");

		char *out_buffer = buffer[current_outbuffer];
		int out_size = buffer_size[current_outbuffer];





// No video.  Put in a fake frame for audio only
		if(!have_video)
		{
#include "data/fake_ntsc_dv.h"
			out_size = sizeof(fake_ntsc_dv) - 4;
			out_buffer = (char*)fake_ntsc_dv + 4;
		}
		unsigned char *output = output_buffer + 
			output_queue.buffer * 
			output_mmap.buf_size;
		int is_pal = (out_size == DV_PAL_SIZE);






// Got a buffer
		if(out_buffer && out_size)
		{
// Calculate number of samples needed based on given pattern for 
// norm.
			int samples_per_frame = 2048;

// Encode audio
			if(audio_samples > samples_per_frame)
			{

				int samples_written = dv_write_audio(encoder,
					(unsigned char*)out_buffer,
					(unsigned char*)audio_buffer,
					samples_per_frame,
					channels,
					bits,
					samplerate,
					is_pal ? DV_PAL : DV_NTSC);
				memcpy(audio_buffer, 
					audio_buffer + samples_written * bits * channels / 8,
					(audio_samples - samples_written) * bits * channels / 8);
				audio_samples -= samples_written;
				position_lock->lock();
				audio_position += samples_written;
				position_lock->unlock();


				audio_lock->unlock();
			}

// Copy from current buffer to mmap buffer with firewire encryption
			encrypt((unsigned char*)output, 
				(unsigned char*)out_buffer, 
				out_size);
			buffer_valid[current_outbuffer] = 0;
		}

// Advance buffer number if possible
		increment_counter(&current_outbuffer);

// Reuse same buffer next time
		if(!buffer_valid[current_outbuffer])
		{
			decrement_counter(&current_outbuffer);
		}
		else
// Wait for user to reach current buffer before unlocking any more.
		{
			video_lock->unlock();
		}


		buffer_lock->unlock();
//printf("Device1394Output::run 100\n");



		if(out_size > 0)
		{

// Write mmap to device
			Thread::enable_cancel();
			unused_buffers--;
			if(ioctl(output_fd, VIDEO1394_TALK_QUEUE_BUFFER, &output_queue) < 0)
			{
        		perror("Device1394Output::run VIDEO1394_TALK_QUEUE_BUFFER");
    		}

    		output_queue.buffer++;
			if(output_queue.buffer >= output_mmap.nb_buffers) 
				output_queue.buffer = 0;

			if(unused_buffers <= 0)
			{
    			if(ioctl(output_fd, VIDEO1394_TALK_WAIT_BUFFER, &output_queue) < 0) 
				{
        			perror("Device1394::run VIDEO1394_TALK_WAIT_BUFFER");
    			}
				unused_buffers++;
			}
			Thread::disable_cancel();
		}
		else
		{
// 			Thread::enable_cancel();
// 			start_lock->lock();
// 			Thread::disable_cancel();
		}

//printf("Device1394Output::run %lld\n", timer.get_difference());
	}
}



void Device1394Output::encrypt(unsigned char *output, 
	unsigned char *data, 
	int data_size)
{
// Encode in IEEE1394 video encryption
	int is_pal = (data_size == DV_PAL_SIZE);
	int output_size = 320;
	int packets_per_frame = (is_pal ? 300 : 250);
	int min_packet_size = output_mmap.packet_size;
	unsigned long frame_size = packets_per_frame * 480;
	unsigned long vdata = 0;
	unsigned int *packet_sizes = this->packet_sizes;

    if(cip_counter == 0) 
	{
        if(!is_pal) 
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
}




void Device1394Output::write_frame(VFrame *input)
{
	VFrame *ptr = 0;
	int is_pal = (input->get_h() == 576);
	int result = 0;

//printf("Device1394Output::write_frame 1\n");

	if(output_fd <= 0) return;
	if(interrupted) return;

// Encode frame to DV
	if(input->get_color_model() != BC_COMPRESSED)
	{
		if(!temp_frame) temp_frame = new VFrame;
		if(!encoder) encoder = dv_new();
		ptr = temp_frame;

// Exact resolution match.  Don't do colorspace conversion
		if(input->get_color_model() == BC_YUV422 &&
			input->get_w() == 720 &&
			(input->get_h() == 480 ||
			input->get_h() == 576))
		{
			int norm = is_pal ? DV_PAL : DV_NTSC;
			int data_size = is_pal ? DV_PAL_SIZE : DV_NTSC_SIZE;
			temp_frame->allocate_compressed_data(data_size);
			temp_frame->set_compressed_size(data_size);

			dv_write_video(encoder,
				temp_frame->get_data(),
				input->get_rows(),
				BC_YUV422,
				norm);
			ptr = temp_frame;
		}
		else
// Convert resolution and color model before compressing
		{
			if(!temp_frame2)
			{
				int h = input->get_h();
// Default to NTSC if unknown
				if(h != 480 && h != 576) h = 480;

				temp_frame2 = new VFrame(0,
					720,
					h,
					BC_YUV422);
				
			}

			int norm = is_pal ? DV_PAL : DV_NTSC;
			int data_size = is_pal ? DV_PAL_SIZE : DV_NTSC_SIZE;
			temp_frame->allocate_compressed_data(data_size);
			temp_frame->set_compressed_size(data_size);


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

			dv_write_video(encoder,
				temp_frame->get_data(),
				temp_frame2->get_rows(),
				BC_YUV422,
				norm);



			ptr = temp_frame;
		}
	}
	else
		ptr = input;











// Take over buffer table
	buffer_lock->lock();
	have_video = 1;
// Wait for buffer to become available with timeout
	while(buffer_valid[current_inbuffer] && !result && !interrupted)
	{
		buffer_lock->unlock();
		result = video_lock->timed_lock(BUFFER_TIMEOUT);
		buffer_lock->lock();
	}



// Write buffer if there's room
	if(!buffer_valid[current_inbuffer])
	{
		if(!buffer[current_inbuffer])
		{
			buffer[current_inbuffer] = new char[ptr->get_compressed_size()];
			buffer_size[current_inbuffer] = ptr->get_compressed_size();
		}
		memcpy(buffer[current_inbuffer], ptr->get_data(), ptr->get_compressed_size());
		buffer_valid[current_inbuffer] = 1;
		increment_counter(&current_inbuffer);
	}
	else
// Ignore it if there isn't room.
	{
		;
	}

	buffer_lock->unlock();
	start_lock->unlock();
//printf("Device1394Output::write_frame 100\n");
}

void Device1394Output::write_samples(char *data, int samples)
{
//printf("Device1394Output::write_samples 1\n");
	int result = 0;
	int timeout = (int64_t)samples * 
		(int64_t)1000000 * 
		(int64_t)2 / 
		(int64_t)samplerate;
	if(interrupted) return;

//printf("Device1394Output::write_samples 2\n");

// Check for maximum sample count exceeded
	if(samples > OUTPUT_SAMPLES)
	{
		printf("Device1394Output::write_samples samples=%d > OUTPUT_SAMPLES=%d\n",
			samples,
			OUTPUT_SAMPLES);
		return;
	}

//printf("Device1394Output::write_samples 3\n");
// Take over buffer table
	buffer_lock->lock();
// Wait for buffer to become available with timeout
	while(audio_samples > OUTPUT_SAMPLES - samples && !result && !interrupted)
	{
		buffer_lock->unlock();
		result = audio_lock->timed_lock(BUFFER_TIMEOUT);
		buffer_lock->lock();
	}

	if(!interrupted && audio_samples <= OUTPUT_SAMPLES - samples)
	{
//printf("Device1394Output::write_samples 4 %d\n", audio_samples);
		memcpy(audio_buffer + audio_samples * channels * bits / 8,
			data,
			samples * channels * bits / 8);
		audio_samples += samples;
	}
	buffer_lock->unlock();
	start_lock->unlock();
//printf("Device1394Output::write_samples 100\n");
}

long Device1394Output::get_audio_position()
{
	position_lock->lock();
	long result = audio_position;
	position_lock->unlock();
	return result;
}

void Device1394Output::interrupt()
{
	interrupted = 1;
// Break write_samples out of a lock
	video_lock->unlock();
	audio_lock->unlock();
// Playback should stop when the object is deleted.
}

void Device1394Output::flush()
{
	
}

void Device1394Output::increment_counter(int *counter)
{
	(*counter)++;
	if(*counter >= total_buffers) *counter = 0;
}

void Device1394Output::decrement_counter(int *counter)
{
	(*counter)--;
	if(*counter < 0) *counter = total_buffers - 1;
}














#endif // HAVE_FIREWIRE






