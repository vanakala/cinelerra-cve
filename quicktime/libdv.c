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
	dv_set_error_log (dv->decoder, 0);
	dv->decoder->quality = DV_QUALITY_BEST;
	dv->decoder->prev_frame_decoded = 0;
	dv->use_mmx = 1;
	return dv;
}


int dv_delete(dv_t *dv)
{
	int i;
	if(dv->decoder)
	{
		dv_decoder_free( dv->decoder );
	}

	if(dv->temp_video)
		free(dv->temp_video);

	if(dv->temp_audio[0])
	{
		for(i = 0; i < 4; i++)
			free(dv->temp_audio[i]);
	}

	if(dv->encoder)
	{
		dv_encoder_free( dv->encoder );
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
		long size,
		int channels,
		int bits)
{
	long current_position;
	int norm;
	int i, j;
    int audio_bytes;
	short *samples_int16 = (short*)samples;
	int samples_read;
	if(channels > 4) channels = 4;

// For some reason someone had problems with libdv's maxmimum audio samples
#define MAX_AUDIO_SAMPLES 2048
	if(!dv->temp_audio[0])
	{
		for(i = 0; i < 4; i++)
			dv->temp_audio[i] = calloc(1, sizeof(int16_t) * MAX_AUDIO_SAMPLES);
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

	for(i = 0; i < channels; i++)
	{
		for(j = 0; j < samples_read; j++)
		{
			samples_int16[i + j * channels] = dv->temp_audio[i][j];
			if(samples_int16[i + j * channels] == -0x8000)
				samples_int16[i + j * channels] = 0;
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
	dv_color_space_t encode_dv_colormodel = 0;

	if(!dv->encoder)
	{
		dv->encoder = dv_encoder_new( 
			0,
			0,
			0);
	}

	switch( color_model )
	{
		case BC_YUV422:
			encode_dv_colormodel = e_dv_color_yuv;
			break;
		case BC_RGB888:
			encode_dv_colormodel = e_dv_color_rgb;
			break;
		default:
			return;
			break;
	}
	dv->encoder->is16x9 = 0;
	dv->encoder->vlc_encode_passes = 3;
	dv->encoder->static_qno = 0;
	dv->encoder->force_dct = DV_DCT_AUTO;
	dv->encoder->isPAL = (norm == DV_PAL);
	
	dv_encode_full_frame( dv->encoder,
		input_rows, 
		encode_dv_colormodel, 
		data );
}



int dv_write_audio(dv_t *dv,
		unsigned char *data,
		unsigned char *input_samples,
		int max_samples,
		int channels,
		int bits,
		int rate,
		int norm)
{
	int i, j;

	if(!dv->encoder)
	{
		dv->encoder = dv_encoder_new( 
			0,
			0,
			0 );
	}
	dv->encoder->isPAL = (norm == DV_PAL);


// Get sample count from a libdv function
	int samples = dv_calculate_samples(dv->encoder, rate, dv->audio_frames);
	dv->audio_frames++;

	if(!dv->temp_audio[0])
	{
		for(i = 0; i < 4; i++)
			dv->temp_audio[i] = calloc(1, sizeof(int16_t) * MAX_AUDIO_SAMPLES);
	}

	for(i = 0; i < channels; i++)
	{
		short *temp_audio = dv->temp_audio[i];
		short *input_channel = (short*)input_samples + i;
		for(j = 0; j < samples; j++)
		{
			temp_audio[j] = input_channel[j * channels];
		}
	}


	dv_encode_full_audio(dv->encoder, 
		dv->temp_audio,
		channels, 
		rate, 
		data);
	return samples;
}


