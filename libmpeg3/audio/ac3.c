#include <stdint.h>
#include <stdio.h>

#include "a52.h"
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

//printf(__FUNCTION__ " %02x%02x%02x%02x%02x%02x%02x%02x\n", header[0], header[1], header[2], header[3], header[4], header[5], header[6], header[7]);
	result = a52_syncinfo(header, 
		&audio->flags,
        &audio->samplerate, 
		&audio->bitrate);


	if(result)
	{
//printf(__FUNCTION__ " %02x%02x%02x%02x%02x%02x%02x%02x\n", header[0], header[1], header[2], header[3], header[4], header[5], header[6], header[7]);
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
				printf("mpeg3_ac3_header: unknown channel code: %p\n", audio->flags & A52_CHANNEL_MASK);
				break;
		}
//printf(__FUNCTION__ " 1 %d\n", audio->channels);
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

//printf("mpeg3audio_doac3 1\n");
	a52_frame(audio->state, 
		frame, 
		&audio->flags,
	   	&level, 
		0);
//printf("mpeg3audio_doac3 2\n");
	a52_dynrng(audio->state, NULL, NULL);
//printf("mpeg3audio_doac3 3\n");
	for(i = 0; i < 6; i++)
	{
		if(!a52_block(audio->state))
		{
			l = 0;
			if(render)
			{
				for(j = 0; j < audio->channels; j++)
				{
					for(k = 0; k < 256; k++)
					{
						output[j][output_position + k] = ((sample_t*)audio->output)[l];
						l++;
					}
				}
			}
			output_position += 256;
		}
	}
//printf("mpeg3audio_doac3 4 %d\n", output_position);


	return output_position;
}



