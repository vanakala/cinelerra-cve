#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <a52dec/a52.h>
#include "mpeg3private.h"
#include "mpeg3protos.h"

#include <string.h>


mpeg3_ac3_t* mpeg3_new_ac3()
{
	mpeg3_ac3_t *result = calloc(1, sizeof(mpeg3_ac3_t));
	result->stream = mpeg3bits_new_stream(0, 0);
	result->state = a52_init(0);
	result->output = a52_samples(result->state);
	return result;
}

void mpeg3_delete_ac3(mpeg3_ac3_t *audio)
{
	mpeg3bits_delete_stream(audio->stream);
	a52_free(audio->state);
	free(audio);
}


/* Return 1 if it isn't an AC3 header */
int mpeg3_ac3_check(unsigned char *header)
{
	int flags, samplerate, bitrate;
	return !a52_syncinfo(header,
		&flags,
		&samplerate,
		&bitrate);
}

/* Decode AC3 header */
int mpeg3_ac3_header(mpeg3_ac3_t *audio, unsigned char *header)
{
	int result = 0;
	audio->flags = 0;

	result = a52_syncinfo(header,
		&audio->flags,
		&audio->samplerate,
		&audio->bitrate);


	if(result)
	{
		audio->framesize = result;
		audio->channels = 0;

		if(audio->flags & A52_LFE)
			audio->channels++;

		switch(audio->flags & A52_CHANNEL_MASK)
		{
			case A52_CHANNEL:
				audio->channels++;
				break;
			case A52_MONO:
				audio->channels++;
				break;
			case A52_STEREO:
				audio->channels += 2;
				break;
			case A52_3F:
				audio->channels += 3;
				break;
			case A52_2F1R:
				audio->channels += 3;
				break;
			case A52_3F1R:
				audio->channels += 4;
				break;
			case A52_2F2R:
				audio->channels += 4;
				break;
			case A52_3F2R:
				audio->channels += 5;
				break;
			case A52_DOLBY:
				audio->channels += 2;
				break;
			default:
				printf("mpeg3_ac3_header: unknown channel code: %x\n", audio->flags & A52_CHANNEL_MASK);
				break;
		}
	}
	return result;
}


int mpeg3audio_doac3(mpeg3_ac3_t *audio, 
	char *frame, 
	int frame_size, 
	float **output,
	int render)
{
	int output_position = 0;
	sample_t level = 1;
	int i, j, k, l;

	a52_frame(audio->state, 
		frame, 
		&audio->flags,
		&level, 
		0);

	a52_dynrng(audio->state, NULL, NULL);

	for(i = 0; i < 6; i++)
	{
		if(!a52_block(audio->state))
		{
			l = 0;
			if(render)
			{
// Remap the channels to conform to encoders.
				for(j = 0; j < audio->channels; j++)
				{
					int dst_channel = j;

// Make LFE last channel.
// Shift all other channels down 1.
					if((audio->flags & A52_LFE))
					{
						if(j == 0)
							dst_channel = audio->channels - 1;
						else
							dst_channel--;
					}

// Swap front left and center for certain configurations
					switch(audio->flags & A52_CHANNEL_MASK)
					{
						case A52_3F:
						case A52_3F1R:
						case A52_3F2R:
							if(dst_channel == 0) dst_channel = 1;
							else
							if(dst_channel == 1) dst_channel = 0;
							break;
					}

					for(k = 0; k < 256; k++)
					{
						output[dst_channel][output_position + k] = ((sample_t*)audio->output)[l];
						l++;
					}
				}
			}
			output_position += 256;
		}
	}


	return output_position;
}

