/*
 * Grabbing algorithm is from dvgrab
 */

#include "colormodels.h"
#include "libdv.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#ifdef USE_MMX
#include "mmx.h"
#endif

#ifdef HAVE_FIREWIRE
static dv_grabber_t *dv_grabber = 0;
#endif


#ifdef HAVE_FIREWIRE
int dv_advance_frame(dv_grabber_t *grabber, int *frame_number)
{
	(*frame_number)++;
	if((*frame_number) >= grabber->frames) (*frame_number) = 0;
	return 0;
}


void dv_reset_keepalive(dv_grabber_t *grabber)
{
	grabber->capturing = 0;
	grabber->still_alive = 1;
}

// From DVGRAB
int dv_iso_handler(raw1394handle_t handle, 
		int channel, 
		size_t length,
		quadlet_t *data)
{
	dv_grabber_t *grabber = dv_grabber;

	if(!grabber) return 0;
	if(grabber->done) return 0;


#define BLOCK_SIZE 480
	if(length > 16)
	{
		unsigned char *ptr = (unsigned char*)&data[3];
		int section_type = ptr[0] >> 5;
		int dif_sequence = ptr[1] >> 4;
		int dif_block = ptr[2];

		if(section_type == 0 && dif_sequence == 0)
		{
// Frame completed
			if(grabber->bytes_read)
			{
// Need to conform the frame size so our format detection is right
				if(grabber->bytes_read == DV_PAL_SIZE ||
					grabber->bytes_read == DV_NTSC_SIZE)
				{
					grabber->frame_size = grabber->bytes_read;
//printf("dv_iso_handler 1 %d\n", grabber->input_frame);
					pthread_mutex_unlock(&grabber->output_lock[grabber->input_frame]);
					dv_advance_frame(grabber, &(grabber->input_frame));
					dv_reset_keepalive(grabber);

					pthread_mutex_lock(&grabber->input_lock[grabber->input_frame]);
					bzero(grabber->frame_buffer[grabber->input_frame], DV_PAL_SIZE);
				}
				grabber->bytes_read = 0;
//printf("dv_iso_handler 2\n");
			}
		}

		switch(section_type)
		{
			case 0: // Header
                memcpy(grabber->frame_buffer[grabber->input_frame] + 
					dif_sequence * 150 * 80, ptr, BLOCK_SIZE);
				break;

			case 1: // Subcode
				memcpy(grabber->frame_buffer[grabber->input_frame] + 
					dif_sequence * 150 * 80 + (1 + dif_block) * 80, 
					ptr, 
					BLOCK_SIZE);
				break;

			case 2: // VAUX
				memcpy(grabber->frame_buffer[grabber->input_frame] + 
					dif_sequence * 150 * 80 + (3 + dif_block) * 80, 
					ptr, 
					BLOCK_SIZE);
				break;

			case 3: // Audio block
				memcpy(grabber->frame_buffer[grabber->input_frame] + 
					dif_sequence * 150 * 80 + (6 + dif_block * 16) * 80, 
					ptr, 
					BLOCK_SIZE);
				break;

			case 4: // Video block
				memcpy(grabber->frame_buffer[grabber->input_frame] + 
					dif_sequence * 150 * 80 + (7 + (dif_block / 15) + dif_block) * 80, 
					ptr, 
					BLOCK_SIZE);
				break;
			
			default:
				break;
		}

		grabber->bytes_read += BLOCK_SIZE;
	}
	return 0;
}


bus_reset_handler_t dv_reset_handler(raw1394handle_t handle, 
	unsigned int generation)
{
	dv_grabber_t *grabber = dv_grabber;
	if(!grabber) return 0;
	if(grabber->done) return 0;
	grabber->crash = 1;

	printf("dv_reset_handler\n");

    return 0;
}

void dv_grabber_thread(dv_grabber_t *grabber)
{
	while(!grabber->done)
	{
//printf("dv_grabber_thread 1 %d\n", grabber->done);
		raw1394_loop_iterate(grabber->handle);
//printf("dv_grabber_thread 2 %d\n", grabber->done);
	}
}

void dv_keepalive_thread(dv_grabber_t *grabber)
{
	while(!grabber->interrupted)
	{
		grabber->still_alive = 0;
		
		grabber->delay.tv_sec = 0;
		grabber->delay.tv_usec = 500000;
		select(0,  NULL,  NULL, NULL, &grabber->delay);
		
		if(grabber->still_alive == 0 && grabber->capturing)
		{
//			printf("dv_keepalive_thread: device crashed\n");
			grabber->crash = 1;
		}
		else
			grabber->crash = 0;
	}
}

// ===================================================================
//                             Entry points
// ===================================================================

// The grabbing algorithm is derived from dvgrab
// http://www.schirmacher.de/arne/dvgrab/.

dv_grabber_t *dv_grabber_new()
{
	dv_grabber_t *grabber;
	grabber = calloc(1, sizeof(dv_grabber_t));
	return grabber;
}

int dv_grabber_delete(dv_grabber_t *grabber)
{
	free(grabber);
	return 0;
}

static int open1394_common(raw1394handle_t *handle, int port, int channel)
{
	int numcards;
    struct raw1394_portinfo pinf[16];

    if(!(*handle = raw1394_new_handle()))
	{
        perror("raw1394_get_handle");
        return 1;
    }

    if((numcards = raw1394_get_port_info(*handle, pinf, 16)) < 0)
	{
        perror("raw1394_get_port_info");
        return 1;
    }

//printf("open1394_common 1 %d\n", numcards);
	if(!pinf[port].nodes)
	{
		printf("open1394_common: pinf[port].nodes == 0\n");
		raw1394_destroy_handle(*handle);
		return 1;
	}

//printf("open1394_common 1 %d\n", pinf[port].nodes);
    if(raw1394_set_port(*handle, port) < 0)
	{
        perror("raw1394_set_port");
		raw1394_destroy_handle(*handle);
        return 1;
    }
	return 0;
}

int dv_start_grabbing(dv_grabber_t *grabber, int port, int channel, int buffers)
{
	int i;
	pthread_attr_t  attr;
	pthread_mutexattr_t mutex_attr;
    int numcards;
    iso_handler_t oldhandler;

	grabber->port = port;
	grabber->channel = channel;
	grabber->frames = buffers;
	grabber->frame_size = DV_PAL_SIZE;
	grabber->input_frame = grabber->output_frame = 0;

	if(open1394_common(&grabber->handle, port, channel))
		return 1;

    oldhandler = raw1394_set_iso_handler(grabber->handle, grabber->channel, dv_iso_handler);
    raw1394_set_bus_reset_handler(grabber->handle, dv_reset_handler);

    if(raw1394_start_iso_rcv(grabber->handle, grabber->channel) < 0)
	{
        printf("dv_start_grabbing: couldn't start iso receive");
		raw1394_destroy_handle(grabber->handle);
        return 1;
    }

	grabber->frame_buffer = calloc(1, grabber->frames * sizeof(unsigned char*));
	grabber->input_lock = calloc(1, grabber->frames * sizeof(pthread_mutex_t));
	grabber->output_lock = calloc(1, grabber->frames * sizeof(pthread_mutex_t));
	pthread_mutexattr_init(&mutex_attr);
	for(i = 0; i < grabber->frames; i++)
	{
		grabber->frame_buffer[i] = calloc(1, DV_PAL_SIZE);

		pthread_mutex_init(&(grabber->input_lock[i]), &mutex_attr);
		pthread_mutex_init(&(grabber->output_lock[i]), &mutex_attr);
		pthread_mutex_lock(&(grabber->output_lock[i]));
	}

	dv_grabber = grabber;
	grabber->done = 0;

	pthread_attr_init(&attr);
// Start keepalive
	grabber->still_alive = 1;
	grabber->interrupted = 0;
	grabber->crash = 0;
	grabber->capturing = 0;
	pthread_create(&(grabber->keepalive_tid), &attr, (void*)dv_keepalive_thread, grabber);

// Start grabber
	pthread_create(&(grabber->tid), &attr, (void*)dv_grabber_thread, grabber);

	return 0;
}

int dv_stop_grabbing(dv_grabber_t* grabber)
{
	int i;

	grabber->done = 1;
	for(i = 0; i < grabber->frames; i++)
	{
		pthread_mutex_unlock(&grabber->input_lock[i]);
	}

//printf("dv_stop_grabbing 1\n");
	pthread_join(grabber->tid, 0);
//printf("dv_stop_grabbing 1.1\n");
	raw1394_stop_iso_rcv(grabber->handle, grabber->channel);
//printf("dv_stop_grabbing 1.2\n");
	raw1394_destroy_handle(grabber->handle);
//printf("dv_stop_grabbing 2\n");

	for(i = 0; i < grabber->frames; i++)
	{
		pthread_mutex_destroy(&(grabber->input_lock[i]));
		pthread_mutex_destroy(&(grabber->output_lock[i]));
		free(grabber->frame_buffer[i]);
	}

	free(grabber->frame_buffer);
	free(grabber->input_lock);
	free(grabber->output_lock);

// Stop keepalive
	grabber->interrupted = 1;
	pthread_cancel(grabber->keepalive_tid);
	pthread_join(grabber->keepalive_tid, 0);
//printf("dv_stop_grabbing 3\n");

	memset(grabber, 0, sizeof(dv_grabber_t));
	return 0;
}

int dv_grabber_crashed(dv_grabber_t* grabber)
{
	if(grabber)
		return grabber->crash;
	else
		return 0;
}

int dv_interrupt_grabber(dv_grabber_t* grabber)
{
	int i;
	
	if(grabber)
	{
		grabber->done = 1;
		pthread_cancel(grabber->tid);
		for(i = 0; i < grabber->frames; i++)
		{
			pthread_mutex_unlock(&(grabber->output_lock[i]));
		}
	}
	return 0;
}

int dv_grab_frame(dv_grabber_t* grabber, unsigned char **frame, long *size)
{
	int result = 0;
	if(!grabber->frame_locked)
	{
// Device failed to open.
// dv_interrupt_grabber should be relied on instead of dv_grab_frame for
// getting out of a crash.
		if(!grabber->frame_buffer)
		{
			*frame = 0;
			*size = 0;
			return 1;
		}

// Wait for frame to become available
		grabber->capturing = 1;
		pthread_mutex_lock(&(grabber->output_lock[grabber->output_frame]));
//printf("dv_grab_frame 1 %d\n", grabber->output_frame);

		if(grabber->done)
		{
			*frame = 0;
			*size = 0;
			return 1;
		}

		*frame = grabber->frame_buffer[grabber->output_frame];
		*size = grabber->frame_size;
		grabber->frame_locked = 1;
	}
	else
		printf("dv_grab_frame: attempted to grab %d before unlocking previous frame.\n", grabber->output_frame);
	return result;
}

int dv_unlock_frame(dv_grabber_t* grabber)
{
	if(grabber->frame_locked)
	{
		pthread_mutex_unlock(&(grabber->input_lock[grabber->output_frame]));
//printf("dv_unlock_frame 2 %d\n", grabber->output_frame);
		dv_advance_frame(grabber, &grabber->output_frame);
//printf("dv_unlock_frame 3 %d\n", grabber->output_frame);
		grabber->frame_locked = 0;
	}
	else
		printf("dv_unlock_frame: attempted to unlock unlocked frame %d\n", grabber->output_frame);
	return 0;
}

dv_playback_t *dv_playback_new()
{
	dv_playback_t *result;
	result = calloc(1, sizeof(dv_playback_t));
	return result;
}

void dv_playback_delete(dv_playback_t *playback)
{
	free(playback);
}

int dv_start_playback(dv_playback_t *playback, int port, int channel)
{
	playback->port = port;
	playback->channel = channel;

	if(open1394_common(&playback->handle, port, channel))
		return 1;
	
	return 0;
}

#endif // HAVE_FIREWIRE

#define DV_WIDTH 720
#define DV_HEIGHT 576


static int dv_initted = 0;
static pthread_mutex_t dv_lock;

dv_t* dv_new()
{
	dv_t *dv = calloc(1, sizeof(dv_t));
	if(!dv_initted)
	{
		pthread_mutexattr_t attr;
		dv_initted = 1;
//		dv_init();
		pthread_mutexattr_init(&attr);
		pthread_mutex_init(&dv_lock, &attr);
	}

	dv->decoder = dv_decoder_new(0, 0, 0);
	dv->decoder->quality = dv->decoder->video->quality;
	dv->decoder->prev_frame_decoded = 0;
	dv->use_mmx = 1;
	return dv;
}


int dv_delete(dv_t *dv)
{
	int i;
	free(dv->decoder->audio);
	free(dv->decoder->video);
	free(dv->decoder);
	if(dv->temp_video)
		free(dv->temp_video);
	if(dv->temp_audio[0])
	{
		for(i = 0; i < 4; i++)
			free(dv->temp_audio[i]);
	}
	free(dv);
	return 0;
}

// Decodes BC_YUV422 only



int dv_read_video(dv_t *dv, 
		unsigned char **output_rows, 
		unsigned char *data, 
		long bytes,
		int color_model)
{
	int dif = 0;
	int lost_coeffs = 0;
	long offset = 0;
	int isPAL = 0;
	int is61834 = 0;
	int numDIFseq;
	int ds;
	int i, v, b, m;
	dv_block_t *bl;
	long mb_offset;
	dv_sample_t sampling;
	dv_macroblock_t *mb;
	int pixel_size;
	int pitches[3];
	int use_temp = color_model != BC_YUV422;
	unsigned char *pixels[3];

//printf("dv_read_video 1 %d\n", color_model);
	pthread_mutex_lock(&dv_lock);
	switch(bytes)
	{
		case DV_PAL_SIZE:
			break;
		case DV_NTSC_SIZE:
			break;
		default:
			return 1;
			break;
	}

	if(data[0] != 0x1f) return 1;

	pitches[0] = DV_WIDTH * 2;
	pitches[1] = 0;
	pitches[2] = 0;
	pixels[1] = 0;
	pixels[2] = 0;

	dv_parse_header(dv->decoder, data);

	if(!use_temp)
	{
//printf("dv_read_video 1\n");
		pixels[0] = output_rows[0];
		dv_decode_full_frame(dv->decoder, 
			data, 
			e_dv_color_yuv, 
			output_rows, 
			pitches);
//printf("dv_read_video 2\n");
	}
	else
	{
		unsigned char *temp_rows[DV_HEIGHT];
		if(!dv->temp_video)
			dv->temp_video = calloc(1, DV_WIDTH * DV_HEIGHT * 2);

		for(i = 0; i < DV_HEIGHT; i++)
		{
			temp_rows[i] = dv->temp_video + i * DV_WIDTH * 2;
		}

		pixels[0] = dv->temp_video;
//printf("dv_read_video 3 %p\n", data);
		dv_decode_full_frame(dv->decoder, 
			data, 
			e_dv_color_yuv, 
			pixels, 
			pitches);
//printf("dv_read_video 4\n");

		cmodel_transfer(output_rows, 
			temp_rows,
			output_rows[0],
			output_rows[1],
			output_rows[2],
			0,
			0,
			0,
			0, 
			0, 
			DV_WIDTH, 
			dv->decoder->height,
			0, 
			0, 
			DV_WIDTH, 
			dv->decoder->height,
			BC_YUV422, 
			color_model,
			0,
			DV_WIDTH,
			DV_WIDTH);
	}
	dv->decoder->prev_frame_decoded = 1;
	pthread_mutex_unlock(&dv_lock);
	return 0;
}






int dv_read_audio(dv_t *dv, 
		unsigned char *samples,
		unsigned char *data,
		long size)
{
	long current_position;
	int norm;
	int i, j;
    int audio_bytes;
	short *samples_int16 = (short*)samples;
	int samples_read;
	
	if(!dv->temp_audio[0])
	{
		for(i = 0; i < 4; i++)
			dv->temp_audio[i] = calloc(1, sizeof(int16_t) * DV_AUDIO_MAX_SAMPLES);
	}

	switch(size)
	{
		case DV_PAL_SIZE:
			norm = DV_PAL;
			break;
		case DV_NTSC_SIZE:
			norm = DV_NTSC;
			break;
		default:
			return 0;
			break;
	}

	if(data[0] != 0x1f) return 0;

	dv_parse_header(dv->decoder, data);
	dv_decode_full_audio(dv->decoder, data, dv->temp_audio);
	samples_read = dv->decoder->audio->samples_this_frame;

	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < samples_read; j++)
		{
			samples_int16[i + j * 2] = dv->temp_audio[i][j];
			if(samples_int16[i + j * 2] == -0x8000)
				samples_int16[i + j * 2] = 0;
		}
	}






	return samples_read;
}





// Encodes BC_YUV422 only

void dv_write_video(dv_t *dv,
		unsigned char *data,
		unsigned char **input_rows,
		int color_model,
		int norm)
{
#if 0
	dv_videosegment_t videoseg ALIGN64;
	int numDIFseq;
	int ds;
	int v;
	unsigned int dif;
	unsigned int offset;
	unsigned char *target = data;
	int isPAL = (norm == DV_PAL);
	time_t now;
	now = time(NULL);
	
	dif = 0;
	offset = dif * 80;
	if (isPAL) 
	{
		target[offset + 3] |= 0x80;
	}

	numDIFseq = isPAL ? 12 : 10;
	
	for (ds = 0; ds < numDIFseq; ds++) 
	{ 






		/* Each DIF segment conists of 150 dif blocks, 
		   135 of which are video blocks */
		/* skip the first 6 dif blocks in a dif sequence */
		dif += 6; 





		/* A video segment consists of 5 video blocks, where each video
		   block contains one compressed macroblock.  DV bit allocation
		   for the VLC stage can spill bits between blocks in the same
		   video segment.  So parsing needs the whole segment to decode
		   the VLC data */
		/* Loop through video segments */
		for (v = 0; v < 27; v++) 
		{
			/* skip audio block - 
			   interleaved before every 3rd video segment
			*/

			if(!(v % 3)) dif++; 

			offset = dif * 80;
			
			videoseg.i = ds;
			videoseg.k = v;
			videoseg.isPAL = isPAL;

			dv_process_videosegment(input_rows, 
					     &videoseg, 
						 target + offset,
					     3,
					     0);
			
			dif += 5;
		} 
	} 
	
	
	
	write_meta_data(target, 0, isPAL, &now);
#endif	
}



void dv_write_audio(dv_t *dv,
		unsigned char *data,
		double *input_samples,
		int channels,
		int norm)
{
}


