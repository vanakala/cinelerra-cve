#include "funcprotos.h"
#include "quicktime.h"
#include "ulaw.h"
#include <stdint.h>

typedef struct
{
	float *ulawtofloat_table;
	float *ulawtofloat_ptr;
	int16_t *ulawtoint16_table;
	int16_t *ulawtoint16_ptr;
	unsigned char *int16toulaw_table;
	unsigned char *int16toulaw_ptr;
	unsigned char *read_buffer;
	long read_size;
} quicktime_ulaw_codec_t;

/* ==================================== private for ulaw */

#define uBIAS 0x84
#define uCLIP 32635

int ulaw_init_ulawtoint16(quicktime_t *file, int track)
{
	int i;
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

/* We use the floating point table to get values for the 16 bit table */
	ulaw_init_ulawtofloat(file, track);
	if(!codec->ulawtoint16_table)
	{
		codec->ulawtoint16_table = malloc(sizeof(int16_t) * 256);
		codec->ulawtoint16_ptr = codec->ulawtoint16_table;

		for(i = 0; i < 256; i++)
		{
			codec->ulawtoint16_table[i] = (int)(32768 * codec->ulawtofloat_ptr[i]);
		}
	}
	return 0;
}

int16_t ulaw_bytetoint16(quicktime_ulaw_codec_t *codec, unsigned char input)
{
	return codec->ulawtoint16_ptr[input];
}

int ulaw_init_ulawtofloat(quicktime_t *file, int track)
{
	int i;
	float value;
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

	if(!codec->ulawtofloat_table)
	{
    	static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
    	int sign, exponent, mantissa, sample;
		unsigned char ulawbyte;

		codec->ulawtofloat_table = malloc(sizeof(float) * 256);
		codec->ulawtofloat_ptr = codec->ulawtofloat_table;
		for(i = 0; i < 256; i++)
		{
			ulawbyte = (unsigned char)i;
    		ulawbyte = ~ulawbyte;
    		sign = (ulawbyte & 0x80);
    		exponent = (ulawbyte >> 4) & 0x07;
    		mantissa = ulawbyte & 0x0F;
    		sample = exp_lut[exponent] + (mantissa << (exponent + 3));
    		if(sign != 0) sample = -sample;

			codec->ulawtofloat_ptr[i] = (float)sample / 32768;
		}
	}
	return 0;
}

float ulaw_bytetofloat(quicktime_ulaw_codec_t *codec, unsigned char input)
{
	return codec->ulawtofloat_ptr[input];
}

int ulaw_init_int16toulaw(quicktime_t *file, int track)
{
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(!codec->int16toulaw_table)
	{
    	int sign, exponent, mantissa;
    	unsigned char ulawbyte;
		int sample;
		int i;
    	int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                               4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

 		codec->int16toulaw_table = malloc(65536);
		codec->int16toulaw_ptr = codec->int16toulaw_table + 32768;

		for(i = -32768; i < 32768; i++)
		{
			sample = i;
/* Get the sample into sign-magnitude. */
    		sign = (sample >> 8) & 0x80;		/* set aside the sign */
    		if(sign != 0) sample = -sample;		/* get magnitude */
    		if(sample > uCLIP) sample = uCLIP;		/* clip the magnitude */

/* Convert from 16 bit linear to ulaw. */
    		sample = sample + uBIAS;
		    exponent = exp_lut[(sample >> 7) & 0xFF];
		    mantissa = (sample >> (exponent + 3)) & 0x0F;
		    ulawbyte = ~(sign | (exponent << 4) | mantissa);
#ifdef ZEROTRAP
		    if (ulawbyte == 0) ulawbyte = 0x02;	    /* optional CCITT trap */
#endif

		    codec->int16toulaw_ptr[i] = ulawbyte;
		}
	}
	return 0;
}

float ulaw_int16tobyte(quicktime_ulaw_codec_t *codec, int16_t input)
{
	return codec->int16toulaw_ptr[input];
}

float ulaw_floattobyte(quicktime_ulaw_codec_t *codec, float input)
{
	return codec->int16toulaw_ptr[(int)(input * 32768)];
}


int ulaw_get_read_buffer(quicktime_t *file, int track, long samples)
{
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

	if(codec->read_buffer && codec->read_size != samples)
	{
		free(codec->read_buffer);
		codec->read_buffer = 0;
	}
	
	if(!codec->read_buffer) 
	{
		int64_t bytes = samples * file->atracks[track].channels;
		codec->read_size = samples;
		if(!(codec->read_buffer = malloc(bytes))) return 1;
	}
	return 0;
}

int ulaw_delete_tables(quicktime_audio_map_t *atrack)
{
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->ulawtofloat_table) 
	{ 
		free(codec->ulawtofloat_table); 
	}
	if(codec->ulawtoint16_table) 
	{ 
		free(codec->ulawtoint16_table); 
	}
	if(codec->int16toulaw_table) 
	{ 
		free(codec->int16toulaw_table); 
	}
	if(codec->read_buffer) free(codec->read_buffer);
	codec->int16toulaw_table = 0;
	codec->ulawtoint16_table = 0;
	codec->ulawtofloat_table = 0;
	codec->read_buffer = 0;
	codec->read_size = 0;
	return 0;
}

/* =================================== public for ulaw */

static int quicktime_delete_codec_ulaw(quicktime_audio_map_t *atrack)
{
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	ulaw_delete_tables(atrack);
	free(codec);
	return 0;
}

static int quicktime_decode_ulaw(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	int result = 0;
	long i;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

	result = ulaw_get_read_buffer(file, track, samples);
	
	if(output_f) result += ulaw_init_ulawtofloat(file, track);
	if(output_i) result += ulaw_init_ulawtoint16(file, track);

	if(!result)
	{
		result = !quicktime_read_audio(file, codec->read_buffer, samples, track);
// Undo increment since this is done in codecs.c
		track_map->current_position -= samples;

/*printf("quicktime_decode_ulaw %d\n", result); */
		if(!result)
		{
			if(output_f)
			{
				unsigned char *input = &(codec->read_buffer[channel]);
				float *output_ptr = output_f;
				float *output_end = output_f + samples;
				int step = file->atracks[track].channels;

				while(output_ptr < output_end)
				{
					*output_ptr++ = ulaw_bytetofloat(codec, *input);
					input += step;
				}
			}
			else
			if(output_i)
			{
				unsigned char *input = &(codec->read_buffer[channel]);
				int16_t *output_ptr = output_i;
				int16_t *output_end = output_i + samples;
				int step = file->atracks[track].channels;

				while(output_ptr < output_end)
				{
					*output_ptr++ = ulaw_bytetoint16(codec, *input);
					input += step;
				}
			}
		}
	}

	return result;
}

static int quicktime_encode_ulaw(quicktime_t *file, 
							int16_t **input_i, 
							float **input_f, 
							int track, 
							long samples)
{
	int result = 0;
	int channel, step;
	int64_t i;
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;
	quicktime_atom_t chunk_atom;
	quicktime_audio_map_t *track_map = &file->atracks[track];
	quicktime_trak_t *trak = track_map->track;

	result = ulaw_init_int16toulaw(file, track);
	result += ulaw_get_read_buffer(file, track, samples);

	if(!result)
	{
		step = file->atracks[track].channels;

		if(input_f)
		{
			for(channel = 0; channel < file->atracks[track].channels; channel++)
			{
				float *input_ptr = input_f[channel];
				float *input_end = input_f[channel] + samples;
				unsigned char *output = codec->read_buffer + channel;

				while(input_ptr < input_end)
				{
					*output = ulaw_floattobyte(codec, *input_ptr++);
					output += step;
				}
			}
		}
		else
		if(input_i)
		{
			for(channel = 0; channel < file->atracks[track].channels; channel++)
			{
				int16_t *input_ptr = input_i[channel];
				int16_t *input_end = input_i[channel] + samples;
				unsigned char *output = codec->read_buffer + channel;

				while(input_ptr < input_end)
				{
					*output = ulaw_int16tobyte(codec, *input_ptr++);
					output += step;
				}
			}
		}

		quicktime_write_chunk_header(file, trak, &chunk_atom);
		result = quicktime_write_data(file, 
			codec->read_buffer, 
			samples * file->atracks[track].channels);
		quicktime_write_chunk_footer(file, 
						trak,
						track_map->current_chunk,
						&chunk_atom, 
						samples);

/* defeat fwrite's return */
		if(result) 
			result = 0; 
		else 
			result = 1; 

		file->atracks[track].current_chunk++;		
	}

	return result;
}


void quicktime_init_codec_ulaw(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_ulaw_codec_t *codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_ulaw_codec_t));
	codec_base->delete_acodec = quicktime_delete_codec_ulaw;
	codec_base->decode_video = 0;
	codec_base->encode_video = 0;
	codec_base->decode_audio = quicktime_decode_ulaw;
	codec_base->encode_audio = quicktime_encode_ulaw;
	codec_base->fourcc = QUICKTIME_ULAW;
	codec_base->title = "uLaw";
	codec_base->desc = "uLaw";
	codec_base->wav_id = 0x07;

/* Init private items */
	codec = ((quicktime_codec_t*)atrack->codec)->priv;
}
