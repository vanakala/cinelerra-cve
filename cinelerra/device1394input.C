#include "condition.h"
#include "device1394input.h"
#include "mutex.h"
#include "vframe.h"
#include "video1394.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define INPUT_SAMPLES 131072
#define BUFFER_TIMEOUT 500000


Device1394Input::Device1394Input()
 : Thread(1, 1, 0)
{
	buffer = 0;
	buffer_valid = 0;
	done = 0;
	total_buffers = 0;
	current_inbuffer = 0;
	current_outbuffer = 0;
	buffer_size = 0;
	temp = 0;
	bytes_read = 0;
	audio_buffer = 0;
	audio_samples = 0;
	video_lock = 0;
	audio_lock = 0;
	buffer_lock = 0;
	handle = 0;
	decoder = 0;
}

Device1394Input::~Device1394Input()
{

// Driver crashes if it isn't stopped before cancelling the thread.
// May still crash during the cancel though.
	if(handle > 0)
	{
		raw1394_stop_iso_rcv(handle, channel);
	}

	if(Thread::running())
	{
		done = 1;
		Thread::cancel();
		Thread::join();
	}

	if(buffer)
	{
		for(int i = 0; i < total_buffers; i++)
			delete [] buffer[i];
		delete [] buffer;
		delete [] buffer_valid;
	}

	if(temp)
	{
		delete [] temp;
	}

	if(audio_buffer)
	{
		delete [] audio_buffer;
	}

	if(handle > 0)
	{
		raw1394_destroy_handle(handle);
	}

	if(decoder)
	{
		dv_delete(decoder);
	}

	if(video_lock) delete video_lock;
	if(audio_lock) delete audio_lock;
	if(buffer_lock) delete buffer_lock;
}

int Device1394Input::open(int port,
	int channel,
	int length,
	int channels,
	int samplerate,
	int bits)
{
	int result = 0;
	this->channel = channel;
	this->length = length;
	this->channels = channels;
	this->samplerate = samplerate;
	this->bits = bits;

// Initialize grabbing
	if(!handle)
	{
		int numcards;
    	struct raw1394_portinfo pinf[16];

    	if(!(handle = raw1394_new_handle()))
		{
        	perror("Device1394::open_input: raw1394_get_handle");
			handle = 0;
        	return 1;
    	}

    	if((numcards = raw1394_get_port_info(handle, pinf, 16)) < 0)
		{
        	perror("Device1394::open_input: raw1394_get_port_info");
 			raw1394_destroy_handle(handle);
			handle = 0;
     	  	return 1;
    	}

		if(!pinf[port].nodes)
		{
			printf("Device1394::open_input: pinf[port].nodes == 0\n");
			raw1394_destroy_handle(handle);
			handle = 0;
			return 1;
		}

    	if(raw1394_set_port(handle, port) < 0)
		{
        	perror("Device1394::open_input: raw1394_set_port");
			raw1394_destroy_handle(handle);
			handle = 0;
        	return 1;
    	}

		raw1394_set_iso_handler(handle, channel, dv_iso_handler);
//		raw1394_set_bus_reset_handler(handle, dv_reset_handler);
		raw1394_set_userdata(handle, this);
    	if(raw1394_start_iso_rcv(handle, channel) < 0)
		{
        	perror("Device1394::open_input: raw1394_start_iso_rcv");
			raw1394_destroy_handle(handle);
			handle = 0;
        	return 1;
    	}





		total_buffers = length;
		buffer = new char*[total_buffers];
		buffer_valid = new int[total_buffers];
		bzero(buffer_valid, sizeof(int) * total_buffers);
		for(int i = 0; i < total_buffers; i++)
		{
			buffer[i] = new char[DV_PAL_SIZE];
		}

		temp = new char[DV_PAL_SIZE];

		audio_buffer = new char[INPUT_SAMPLES * 2 * channels];

		audio_lock = new Condition(0);
		video_lock = new Condition(0);
		buffer_lock = new Mutex;

		decoder = dv_new();

		Thread::start();
	}
	return result;
}

void Device1394Input::run()
{
	while(!done)
	{
		Thread::enable_cancel();
		raw1394_loop_iterate(handle);
		Thread::disable_cancel();
	}
}

void Device1394Input::increment_counter(int *counter)
{
	(*counter)++;
	if(*counter >= total_buffers) *counter = 0;
}

void Device1394Input::decrement_counter(int *counter)
{
	(*counter)--;
	if(*counter < 0) *counter = total_buffers - 1;
}



int Device1394Input::read_video(VFrame *data)
{
	int result = 0;

//printf("Device1394Input::read_video 1\n");
// Take over buffer table
	buffer_lock->lock();
//printf("Device1394Input::read_video 1\n");
// Wait for buffer with timeout
	while(!buffer_valid[current_outbuffer] && !result)
	{
		buffer_lock->unlock();
		result = video_lock->timed_lock(BUFFER_TIMEOUT);
		buffer_lock->lock();
	}
//printf("Device1394Input::read_video 1\n");

// Copy frame
	if(buffer_valid[current_outbuffer])
	{
		data->allocate_compressed_data(buffer_size);
		data->set_compressed_size(buffer_size);
		memcpy(data->get_data(), buffer[current_outbuffer], buffer_size);
		buffer_valid[current_outbuffer] = 0;
		increment_counter(&current_outbuffer);
	}
//printf("Device1394Input::read_video 100\n");

	buffer_lock->unlock();
	return result;
}




int Device1394Input::read_audio(char *data, int samples)
{
	int result = 0;
//printf("Device1394Input::read_audio 1\n");
	int timeout = (int64_t)samples * (int64_t)1000000 * (int64_t)2 / (int64_t)samplerate;
	if(timeout < 500000) timeout = 500000;
//printf("Device1394Input::read_audio 1\n");

// Take over buffer table
	buffer_lock->lock();
// Wait for buffer with timeout
	while(audio_samples < samples && !result)
	{
		buffer_lock->unlock();
		result = audio_lock->timed_lock(timeout);
		buffer_lock->lock();
	}
//printf("Device1394Input::read_audio 1 %d %d\n", result, timeout);

	if(audio_samples >= samples)
	{
		memcpy(data, audio_buffer, samples * bits * channels / 8);
		memcpy(audio_buffer, 
			audio_buffer + samples * bits * channels / 8,
			(audio_samples - samples) * bits * channels / 8);
		audio_samples -= samples;
	}
//printf("Device1394Input::read_audio 100\n");
	buffer_lock->unlock();
	return result;
}






int Device1394Input::dv_iso_handler(raw1394handle_t handle, 
		int channel, 
		size_t length,
		quadlet_t *data)
{
	Device1394Input *thread = (Device1394Input*)raw1394_get_userdata(handle);

//printf("Device1394Input::dv_iso_handler 1\n");
	thread->Thread::disable_cancel();

#define BLOCK_SIZE 480
	if(length > 16)
	{
		unsigned char *ptr = (unsigned char*)&data[3];
		int section_type = ptr[0] >> 5;
		int dif_sequence = ptr[1] >> 4;
		int dif_block = ptr[2];

// Frame completed
		if(section_type == 0 && 
			dif_sequence == 0 && 
			thread->bytes_read)
		{
// Need to conform the frame size so our format detection is right
//printf("Device1394Input::dv_iso_handler 10\n");
			if(thread->bytes_read == DV_PAL_SIZE ||
				thread->bytes_read == DV_NTSC_SIZE)
			{
// Copy frame to buffer
//printf("Device1394Input::dv_iso_handler 20\n");
				thread->buffer_size = thread->bytes_read;


				thread->buffer_lock->lock();
//printf("Device1394Input::dv_iso_handler 21 %p\n", thread->buffer[thread->current_inbuffer]);
				memcpy(thread->buffer[thread->current_inbuffer],
					thread->temp,
					thread->bytes_read);
//printf("Device1394Input::dv_iso_handler 22 %p\n", thread->buffer[thread->current_inbuffer]);
				thread->buffer_valid[thread->current_inbuffer] = 1;
				thread->video_lock->unlock();

// Decode audio to audio store
				if(thread->audio_samples < INPUT_SAMPLES - 2048)
				{
					int decode_result = dv_read_audio(thread->decoder, 
						(unsigned char*)thread->audio_buffer + 
							thread->audio_samples * 2 * 2,
						(unsigned char*)thread->temp,
						thread->bytes_read,
						thread->channels,
						thread->bits);
					thread->audio_samples += decode_result;

//printf("Device1394Input::dv_iso_handler 25 %d\n", decode_result);
					thread->audio_lock->unlock();
				}

// Advance buffer if possible
				thread->increment_counter(&thread->current_inbuffer);
				if(thread->buffer_valid[thread->current_inbuffer])
					thread->decrement_counter(&thread->current_inbuffer);

				thread->buffer_lock->unlock();
//printf("Device1394Input::dv_iso_handler 30\n");
			}
			thread->bytes_read = 0;
		}

//printf("Device1394Input::dv_iso_handler 40\n");
		switch(section_type)
		{
			case 0: // Header
                memcpy(thread->temp + 
					dif_sequence * 150 * 80, ptr, BLOCK_SIZE);
				break;

			case 1: // Subcode
				memcpy(thread->temp + 
					dif_sequence * 150 * 80 + (1 + dif_block) * 80, 
					ptr, 
					BLOCK_SIZE);
				break;

			case 2: // VAUX
				memcpy(thread->temp + 
					dif_sequence * 150 * 80 + (3 + dif_block) * 80, 
					ptr, 
					BLOCK_SIZE);
				break;

			case 3: // Audio block
				memcpy(thread->temp + 
					dif_sequence * 150 * 80 + (6 + dif_block * 16) * 80, 
					ptr, 
					BLOCK_SIZE);
				break;

			case 4: // Video block
				memcpy(thread->temp + 
					dif_sequence * 150 * 80 + (7 + (dif_block / 15) + dif_block) * 80, 
					ptr, 
					BLOCK_SIZE);
				break;
			
			default:
				break;
		}
//printf("Device1394Input::dv_iso_handler 50\n");

		thread->bytes_read += BLOCK_SIZE;
	}
//printf("Device1394Input::dv_iso_handler 100\n");

	return 0;
}

bus_reset_handler_t Device1394Input::dv_reset_handler(raw1394handle_t handle, 
	unsigned int generation)
{
	printf("Device1394::dv_reset_handler: generation=%p\n", generation);
    return 0;
}
