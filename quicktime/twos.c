#include "funcprotos.h"
#include "quicktime.h"
#include "twos.h"

/* =================================== private for twos */

static int byte_order(void)
{                /* 1 if little endian */
	int16_t byteordertest;
	int byteorder;

	byteordertest = 0x0001;
	byteorder = *((unsigned char *)&byteordertest);
	return byteorder;
}

static int swap_bytes(char *buffer, long samples, int channels, int bits)
{
	long i = 0;
	char byte1, byte2, byte3;
	char *buffer1, *buffer2, *buffer3;

	if(!byte_order()) return 0;

	switch(bits)
	{
		case 8:
			break;

		case 16:
			buffer1 = buffer;
			buffer2 = buffer + 1;
			while(i < samples * 2)
			{
				byte1 = buffer2[i];
				buffer2[i] = buffer1[i];
				buffer1[i] = byte1;
				i += 2;
			}
			break;

		case 24:
			buffer1 = buffer;
			buffer2 = buffer + 2;
			while(i < samples * 3)
			{
				byte1 = buffer2[i];
				buffer2[i] = buffer1[i];
				buffer1[i] = byte1;
				i += 3;
			}
			break;

		default:
			break;
	}
	return 0;
}

static int get_work_buffer(quicktime_t *file, int track, long bytes)
{
	quicktime_twos_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

	if(codec->work_buffer && codec->buffer_size != bytes)
	{
		free(codec->work_buffer);
		codec->work_buffer = 0;
	}
	
	if(!codec->work_buffer) 
	{
		codec->buffer_size = bytes;
		if(!(codec->work_buffer = malloc(bytes))) return 1;
	}
	return 0;
}

/* =================================== public for twos */

static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_twos_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->work_buffer) free(codec->work_buffer);
	codec->work_buffer = 0;
	codec->buffer_size = 0;
	free(codec);
	return 0;
}

static int decode(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{

//printf("decode 1\n");
	int result = 0;
	long i, j;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_twos_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int step = file->atracks[track].channels * quicktime_audio_bits(file, track) / 8;

//printf("decode 2\n");
	get_work_buffer(file, track, samples * step);
	result = !quicktime_read_audio(file, codec->work_buffer, samples, track);

//printf("decode 3\n");

// Undo increment since this is done in codecs.c
	track_map->current_position -= samples;

//printf("decode 4\n");
	switch(quicktime_audio_bits(file, track))
	{
		case 8:
			if(output_i && !result)
			{
				for(i = 0, j = channel; i < samples; i++)
				{
					output_i[i] = ((int16_t)codec->work_buffer[j]) << 8;
					j += step;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = channel; i < samples; i++)
				{
					output_f[i] = ((float)codec->work_buffer[j]) / 0x7f;
					j += step;
				}
			}
			break;
		
		case 16:
			if(output_i && !result)
			{
				for(i = 0, j = channel * 2; i < samples; i++)
				{
//printf("decode 5\n");
					output_i[i] = ((int16_t)codec->work_buffer[j]) << 8 |
									((unsigned char)codec->work_buffer[j + 1]);
//printf("decode 6\n");
					j += step;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = channel * 2; i < samples; i++)
				{
//printf("decode 7\n");
					output_f[i] = (float)((((int16_t)codec->work_buffer[j]) << 8) |
									((unsigned char)codec->work_buffer[j + 1])) / 0x7fff;
//printf("decode 8\n");
					j += step;
				}
			}
			break;
		
		case 24:
			if(output_i && !result)
			{
				for(i = 0, j = channel * 3; i < samples; i++)
				{
					output_i[i] = (((int16_t)codec->work_buffer[j]) << 8) | 
									((unsigned char)codec->work_buffer[j + 1]);
					j += step;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = channel * 3; i < samples; i++)
				{
					output_f[i] = (float)((((int)codec->work_buffer[j]) << 16) | 
									(((unsigned char)codec->work_buffer[j + 1]) << 8) |
									((unsigned char)codec->work_buffer[j + 2])) / 0x7fffff;
					j += step;
				}
			}
			break;
		
		default:
			break;
	}
//printf("decode 9\n");

	return result;
}

#define CLAMP(x, y, z) ((x) = ((x) <  (y) ? (y) : ((x) > (z) ? (z) : (x))))

static int encode(quicktime_t *file, 
							int16_t **input_i, 
							float **input_f, 
							int track, 
							long samples)
{
	int result = 0;
	long i, j, offset;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_twos_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int step = file->atracks[track].channels * quicktime_audio_bits(file, track) / 8;
	int sample;
	float sample_f;

	get_work_buffer(file, track, samples * step);

	if(input_i)
	{
		for(i = 0; i < track_map->channels; i++)
		{
			switch(quicktime_audio_bits(file, track))
			{
				case 8:
					for(j = 0; j < samples; j++)
					{
						sample = input_i[i][j] >> 8;
						codec->work_buffer[j * step + i] = sample;
					}
					break;
				case 16:
					for(j = 0; j < samples; j++)
					{
						sample = input_i[i][j];
						codec->work_buffer[j * step + i * 2] = ((unsigned int)sample & 0xff00) >> 8;
						codec->work_buffer[j * step + i * 2 + 1] = ((unsigned int)sample) & 0xff;
					}
					break;
				case 24:
					for(j = 0; j < samples; j++)
					{
						sample = input_i[i][j];
						codec->work_buffer[j * step + i * 3] = ((unsigned int)sample & 0xff00) >> 8;
						codec->work_buffer[j * step + i * 3 + 1] = ((unsigned int)sample & 0xff);
						codec->work_buffer[j * step + i * 3 + 2] = 0;
					}
					break;
			}
		}
	}
	else
	{
		for(i = 0; i < track_map->channels; i++)
		{
			switch(quicktime_audio_bits(file, track))
			{
				case 8:
					for(j = 0; j < samples; j++)
					{
						sample_f = input_f[i][j];
						if(sample_f < 0)
							sample = (int)(sample_f * 0x7f - 0.5);
						else
							sample = (int)(sample_f * 0x7f + 0.5);
						CLAMP(sample, -0x7f, 0x7f);
						codec->work_buffer[j * step + i] = sample;
					}
					break;
				case 16:
					for(j = 0; j < samples; j++)
					{
						sample_f = input_f[i][j];
						if(sample_f < 0)
							sample = (int)(sample_f * 0x7fff - 0.5);
						else
							sample = (int)(sample_f * 0x7fff + 0.5);
						CLAMP(sample, -0x7fff, 0x7fff);
						codec->work_buffer[j * step + i * 2] = ((unsigned int)sample & 0xff00) >> 8;
						codec->work_buffer[j * step + i * 2 + 1] = ((unsigned int)sample) & 0xff;
					}
					break;
				case 24:
					for(j = 0; j < samples; j++)
					{
						sample_f = input_f[i][j];
						if(sample_f < 0)
							sample = (int)(sample_f * 0x7fffff - 0.5);
						else
							sample = (int)(sample_f * 0x7fffff + 0.5);
						CLAMP(sample, -0x7fffff, 0x7fffff);
						codec->work_buffer[j * step + i * 3] = ((unsigned int)sample & 0xff0000) >> 16;
						codec->work_buffer[j * step + i * 3 + 1] = ((unsigned int)sample & 0xff00) >> 8;
						codec->work_buffer[j * step + i * 3 + 2] = ((unsigned int)sample) & 0xff;
					}
					break;
			}
		}
	}

	result = quicktime_write_audio(file, codec->work_buffer, samples, track);

	return result;
}


void quicktime_init_codec_twos(quicktime_audio_map_t *atrack)
{
	quicktime_twos_codec_t *codec;

/* Init public items */
	((quicktime_codec_t*)atrack->codec)->priv = calloc(1, sizeof(quicktime_twos_codec_t));
	((quicktime_codec_t*)atrack->codec)->delete_acodec = delete_codec;
	((quicktime_codec_t*)atrack->codec)->decode_video = 0;
	((quicktime_codec_t*)atrack->codec)->encode_video = 0;
	((quicktime_codec_t*)atrack->codec)->decode_audio = decode;
	((quicktime_codec_t*)atrack->codec)->encode_audio = encode;

/* Init private items */
	codec = ((quicktime_codec_t*)atrack->codec)->priv;
	codec->work_buffer = 0;
	codec->buffer_size = 0;
}
