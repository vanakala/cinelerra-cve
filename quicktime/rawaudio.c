#include "quicktime.h"
#include "rawaudio.h"

typedef struct
{
	char *work_buffer;
	long buffer_size;
} quicktime_rawaudio_codec_t;

/* =================================== private for rawaudio */

int rawaudio_byte_order(void)
{                /* 1 if little endian */
	int16_t byteordertest;
	int byteorder;

	byteordertest = 0x0001;
	byteorder = *((unsigned char *)&byteordertest);
	return byteorder;
}

int rawaudio_swap_bytes(char *buffer, long samples, int channels, int bits)
{
	long i;
	char byte1, byte2, byte3;
	char *buffer1, *buffer2, *buffer3;

	if(!rawaudio_byte_order()) return 0;

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
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

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

/* =================================== public for rawaudio */

static int quicktime_delete_codec_rawaudio(quicktime_audio_map_t *atrack)
{
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->work_buffer) free(codec->work_buffer);
	codec->work_buffer = 0;
	codec->buffer_size = 0;
	free(codec);
	return 0;
}

static int quicktime_decode_rawaudio(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	int result = 0;
	long i, j;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int step = file->atracks[track].channels * quicktime_audio_bits(file, track) / 8;

	get_work_buffer(file, track, samples * step);
	result = !quicktime_read_audio(file, codec->work_buffer, samples, track);
// Undo increment since this is done in codecs.c
	track_map->current_position -= samples;

	switch(quicktime_audio_bits(file, track))
	{
		case 8:
			if(output_i && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_i[i] = ((int16_t)((unsigned char)codec->work_buffer[j]) << 8);
					j += step;
					output_i[i] -= 0x8000;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_f[i] = (float)((unsigned char)codec->work_buffer[j]) - 0x80;
					output_f[i] /= 0x7f;
					j += step;
				}
			}
			break;

		case 16:
			if(output_i && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_i[i] = (int16_t)(codec->work_buffer[j]) << 8 |
									(unsigned char)(codec->work_buffer[j + 1]);
					j += step;
					output_i[i] -= 0x8000;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_f[i] = (float)((int16_t)(codec->work_buffer[j]) << 8 |
									(unsigned char)(codec->work_buffer[j + 1])) - 0x8000;
					output_f[i] /= 0x7fff;
					j += step;
				}
			}
			break;

		case 24:
			if(output_i && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_i[i] = ((int16_t)(codec->work_buffer[j]) << 8) | 
									(unsigned char)(codec->work_buffer[j + 1]);
					output_i[i] -= 0x8000;
					j += step;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_f[i] = (float)(((int)(codec->work_buffer[j]) << 16) | 
									((unsigned int)(codec->work_buffer[j + 1]) << 8) |
									(unsigned char)(codec->work_buffer[j + 2])) - 0x800000;
					output_f[i] /= 0x7fffff;
					j += step;
				}
			}
			break;

		default:
			break;
	}
/*printf("quicktime_decode_rawaudio 2\n"); */

	return result;
}

#define CLAMP(x, y, z) ((x) = ((x) <  (y) ? (y) : ((x) > (z) ? (z) : (x))))

static int quicktime_encode_rawaudio(quicktime_t *file, 
							int16_t **input_i, 
							float **input_f, 
							int track, 
							long samples)
{
	int result = 0;
	long i, j, offset;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
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
						sample += 0x80;
						codec->work_buffer[j * step + i] = sample;
					}
					break;
				case 16:
					for(j = 0; j < samples; j++)
					{
						sample = input_i[i][j];
						sample += 0x8000;
						codec->work_buffer[j * step + i * 2] = ((unsigned int)sample & 0xff00) >> 8;
						codec->work_buffer[j * step + i * 2 + 1] = ((unsigned int)sample) & 0xff;
					}
					break;
				case 24:
					for(j = 0; j < samples; j++)
					{
						sample = input_i[i][j];
						sample += 0x8000;
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
						sample += 0x80;
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
						sample += 0x8000;
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
						sample += 0x800000;
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


void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_rawaudio_codec_t *codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_rawaudio_codec_t));
	codec_base->delete_acodec = quicktime_delete_codec_rawaudio;
	codec_base->decode_video = 0;
	codec_base->encode_video = 0;
	codec_base->decode_audio = quicktime_decode_rawaudio;
	codec_base->encode_audio = quicktime_encode_rawaudio;
	codec_base->fourcc = QUICKTIME_RAW;
	codec_base->title = "8 bit unsigned";
	codec_base->desc = "8 bit unsigned for video";
	codec_base->wav_id = 0x01;

/* Init private items */
	codec = codec_base->priv;
}
