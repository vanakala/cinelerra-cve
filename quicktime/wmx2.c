#include "qtprivate.h"
#include "quicktime.h"
#include "wmx2.h"

typedef struct
{
/* During decoding the work_buffer contains the most recently read chunk. */
/* During encoding the work_buffer contains interlaced overflow samples  */
/* from the last chunk written. */
	int16_t *write_buffer;
	unsigned char *read_buffer;    /* Temporary buffer for drive reads. */

/* Starting information for all channels during encoding a chunk. */
	int *last_samples, *last_indexes;
	long chunk; /* Number of chunk in work buffer */
	int buffer_channel; /* Channel of work buffer */

/* Number of samples in largest chunk read. */
/* Number of samples plus overflow in largest chunk write, interlaced. */
	long write_size;     /* Size of write buffer. */
	long read_size;     /* Size of read buffer. */
} quicktime_wmx2_codec_t;

static int quicktime_wmx2_step[89] = 
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static int quicktime_wmx2_index[16] = 
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

/* ================================== private for wmx2 */
#define HEADER_SIZE 3

static int wmx2_decode_sample(int *predictor, int *nibble, int *index, int *step)
{
	int difference, sign;

/* Get new index value */
	*index += quicktime_wmx2_index[*nibble];

	if(*index < 0) *index = 0; 
	else 
	if(*index > 88) *index = 88;

/* Get sign and magnitude from *nibble */
	sign = *nibble & 8;
	*nibble = *nibble & 7;

/* Get difference */
	difference = *step >> 3;
	if(*nibble & 4) difference += *step;
	if(*nibble & 2) difference += *step >> 1;
	if(*nibble & 1) difference += *step >> 2;

/* Predict value */
	if(sign) 
	*predictor -= difference;
	else 
	*predictor += difference;

	if(*predictor > 32767) *predictor = 32767;
	else
	if(*predictor < -32768) *predictor = -32768;

/* Update the step value */
	*step = quicktime_wmx2_step[*index];

	return 0;
}


static int wmx2_decode_block(quicktime_audio_map_t *atrack, int16_t *output, unsigned char *input, int samples)
{
	int predictor;
	int index;
	int step;
	int i, nibble, nibble_count, block_size;
	unsigned char *block_ptr;
	int16_t *output_end = output + samples;
	quicktime_wmx2_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

/* Get the chunk header */
	predictor = *input++ << 8;
	predictor |= *input++;
	if(predictor & 0x8000) predictor -= 0x10000;
	index = *input++;
	if(index > 88) index = 88;

/*printf("input %d %d\n", predictor, index); */
	step = quicktime_wmx2_step[index];

/* Read the input buffer sequentially, one nibble at a time */
	nibble_count = 0;
	while(output < output_end)
	{
		nibble = nibble_count ? (*input++  >> 4) & 0x0f : *input & 0x0f;

		wmx2_decode_sample(&predictor, &nibble, &index, &step);
		*output++ = predictor;

		nibble_count ^= 1;
	}
}

static int wmx2_encode_sample(int *last_sample, int *last_index, int *nibble, int next_sample)
{
	int difference, new_difference, mask, step;

	difference = next_sample - *last_sample;
	*nibble = 0;
	step = quicktime_wmx2_step[*last_index];
	new_difference = step >> 3;

	if(difference < 0)
	{
		*nibble = 8;
		difference = -difference;
	}

	mask = 4;
	while(mask)
	{
		if(difference >= step)
		{
			*nibble |= mask;
			difference -= step;
			new_difference += step;
		}

		step >>= 1;
		mask >>= 1;
	}

	if(*nibble & 8)
		*last_sample -= new_difference;
	else
		*last_sample += new_difference;

	if(*last_sample > 32767) *last_sample = 32767;
	else
	if(*last_sample < -32767) *last_sample = -32767;

	*last_index += quicktime_wmx2_index[*nibble];

	if(*last_index < 0) *last_index = 0;
	else
	if(*last_index > 88) *last_index= 88;

	return 0;
}

static int wmx2_encode_block(quicktime_audio_map_t *atrack, unsigned char *output, int16_t *input, int step, int channel, int samples)
{
	quicktime_wmx2_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
	int i, nibble_count = 0, nibble, header;

/* Get a fake starting sample */
	header = codec->last_samples[channel];
/*printf("output %d %d\n", header, codec->last_indexes[channel]); */
/* Force rounding. */
/*	if(header < 0x7fc0) header += 0x40; */
/*	header &= 0xff80; */
	if(header < 0) header += 0x10000;
	*output++ = (header & 0xff00) >> 8;
	*output++ = (header & 0xff);
	*output++ = (codec->last_indexes[channel] & 0x7f);

	for(i = 0; i < samples; i++)
	{
		wmx2_encode_sample(&(codec->last_samples[channel]), 
							&(codec->last_indexes[channel]), 
							&nibble, 
							*input);

		if(nibble_count)
			*output++ |= (nibble << 4);
		else
			*output = nibble;

		input += step;
		nibble_count ^= 1;
	}

	return 0;
}

/* Convert the number of samples in a chunk into the number of bytes in that */
/* chunk.  The number of samples in a chunk should end on a block boundary. */
static long wmx2_samples_to_bytes(long samples, int channels)
{
	long bytes = samples / 2;
	if(bytes * 2 < samples) bytes++;
	bytes *= channels;
	bytes += HEADER_SIZE * channels;
	return bytes;
}

/* Decode the chunk into the work buffer */
static int wmx2_decode_chunk(quicktime_t *file, int track, long chunk, int channel)
{
	int result = 0;
	int i, j;
	long chunk_samples, chunk_bytes;
	unsigned char *chunk_ptr, *block_ptr;
	quicktime_trak_t *trak = file->atracks[track].track;
	quicktime_wmx2_codec_t *codec = 
		((quicktime_codec_t*)file->atracks[track].codec)->priv;

/* Get the byte count to read. */
	chunk_samples = quicktime_chunk_samples(trak, chunk);
	chunk_bytes = wmx2_samples_to_bytes(chunk_samples, file->atracks[track].channels);

/* Get the buffer to read into. */
	if(codec->write_buffer && codec->write_size < chunk_samples)
	{
		free(codec->write_buffer);
		codec->write_buffer = 0;
	}

	if(!codec->write_buffer)
	{
		codec->write_size = chunk_samples;
		codec->write_buffer = malloc(sizeof(int16_t) * codec->write_size);
	}

	if(codec->read_buffer && codec->read_size < chunk_bytes)
	{
		free(codec->read_buffer);
		codec->read_buffer = 0;
	}

	if(!codec->read_buffer)
	{
		codec->read_size = chunk_bytes;
		codec->read_buffer = malloc(codec->read_size);
	}

/* codec->work_size now holds the number of samples in the last chunk */
/* codec->read_size now holds number of bytes in the last read buffer */

/* Read the entire chunk regardless of where the desired sample range starts. */
	result = quicktime_read_chunk(file, codec->read_buffer, track, chunk, 0, chunk_bytes);

/* Now decode the chunk, one block at a time, until the total samples in the chunk */
/* is reached. */

	if(!result)
	{
		block_ptr = codec->read_buffer;
		for(j = 0; j < file->atracks[track].channels; j++)
		{
			if(j == channel)
				wmx2_decode_block(&(file->atracks[track]), codec->write_buffer, block_ptr, chunk_samples);

			block_ptr += chunk_bytes / file->atracks[track].channels;
		}
	}
	codec->buffer_channel = channel;
	codec->chunk = chunk;

	return result;
}


/* =================================== public for wmx2 */

static int decode(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f,
					long samples, 
					int track, 
					int channel)
{
	int result = 0;
	long chunk, chunk_sample, chunk_bytes, chunk_samples;
	long i, chunk_start, chunk_end;
	quicktime_trak_t *trak = file->atracks[track].track;
	quicktime_wmx2_codec_t *codec = 
		((quicktime_codec_t*)file->atracks[track].codec)->priv;

/* Get the first chunk with this routine and then increase the chunk number. */
	quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, file->atracks[track].current_position);

/* Read chunks until the output is full. */
	for(i = 0; i < samples && !result; )
	{
/* Get chunk we're on. */
		chunk_samples = quicktime_chunk_samples(trak, chunk);

		if(!codec->write_buffer ||
			codec->chunk != chunk ||
			codec->buffer_channel != channel)
		{
/* read a new chunk if necessary */
			result = wmx2_decode_chunk(file, track, chunk, channel);
		}

/* Get boundaries from the chunk */
		chunk_start = 0;
		if(chunk_sample < file->atracks[track].current_position)
			chunk_start = file->atracks[track].current_position - chunk_sample;

		chunk_end = chunk_samples;
		if(chunk_sample + chunk_end > file->atracks[track].current_position + samples)
			chunk_end = file->atracks[track].current_position + samples - chunk_sample;

/* Read from the chunk */
		if(output_i)
		{
			while(chunk_start < chunk_end)
			{
				output_i[i++] = codec->write_buffer[chunk_start++];
			}
		}
		else
		if(output_f)
		{
			while(chunk_start < chunk_end)
			{
				output_f[i++] = (float)codec->write_buffer[chunk_start++] / 32767;
			}
		}

		chunk++;
		chunk_sample += chunk_samples;
	}

	return result;
}

static int encode(quicktime_t *file, 
						int16_t **input_i, 
						float **input_f, 
						int track, 
						long samples)
{
	int result = 0;
	long i, j, step;
	long chunk_bytes;
	long offset;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_wmx2_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_trak_t *trak = track_map->track;
	int16_t *input_ptr;
	unsigned char *output_ptr;
	quicktime_atom_t chunk_atom;

/* Get buffer sizes */
	if(codec->write_buffer && codec->write_size < samples * track_map->channels)
	{
/* Create new buffer */
		long new_size = samples * track_map->channels;
		int16_t *new_buffer = malloc(sizeof(int16_t) * new_size);

/* Swap pointers. */
		free(codec->write_buffer);
		codec->write_buffer = new_buffer;
		codec->write_size = new_size;
	}
	else
	if(!codec->write_buffer)
	{
/* No buffer in the first place. */
		codec->write_size = samples * track_map->channels;
		codec->write_buffer = malloc(sizeof(int16_t) * codec->write_size);
	}

/* Get output size */
	chunk_bytes = wmx2_samples_to_bytes(samples, track_map->channels);
	if(codec->read_buffer && codec->read_size < chunk_bytes)
	{
		free(codec->read_buffer);
		codec->read_buffer = 0;
	}

	if(!codec->read_buffer)
	{
		codec->read_buffer = malloc(chunk_bytes);
		codec->read_size = chunk_bytes;
	}

	if(!codec->last_samples)
	{
		codec->last_samples = malloc(sizeof(int) * track_map->channels);
		for(i = 0; i < track_map->channels; i++)
		{
			codec->last_samples[i] = 0;
		}
	}

	if(!codec->last_indexes)
	{
		codec->last_indexes = malloc(sizeof(int) * track_map->channels);
		for(i = 0; i < track_map->channels; i++)
		{
			codec->last_indexes[i] = 0;
		}
	}

/* Arm the input buffer */
	step = track_map->channels;
	for(j = 0; j < track_map->channels; j++)
	{
		input_ptr = codec->write_buffer + j;

		if(input_i)
		{
			for(i = 0; i < samples; i++)
			{
				*input_ptr = input_i[j][i];
				input_ptr += step;
			}
		}
		else
		if(input_f)
		{
			for(i = 0; i < samples; i++)
			{
				*input_ptr = (int16_t)(input_f[j][i] * 32767);
				input_ptr += step;
			}
		}
	}

/* Encode from the input buffer to the read_buffer. */
	input_ptr = codec->write_buffer;
	output_ptr = codec->read_buffer;

	for(j = 0; j < track_map->channels; j++)
	{
		wmx2_encode_block(track_map, output_ptr, input_ptr + j, track_map->channels, j, samples);

		output_ptr += chunk_bytes / track_map->channels;
	}

	if(samples)
	{
		offset = quicktime_position(file);
		quicktime_write_chunk_header(file, trak, &chunk_atom);
		result = quicktime_write_data(file, codec->read_buffer, chunk_bytes);
		if(result)
			result = 0; 
		else 
			result = 1; /* defeat fwrite's return */
		quicktime_write_chunk_footer(file, 
			trak,
			track_map->current_chunk,
			&chunk_atom, 
			1);
		track_map->current_chunk++;
	}

	return result;
}

static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_wmx2_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
	if(codec->write_buffer) free(codec->write_buffer);
	if(codec->read_buffer) free(codec->read_buffer);
	if(codec->last_samples) free(codec->last_samples);
	if(codec->last_indexes) free(codec->last_indexes);
	codec->last_samples = 0;
	codec->last_indexes = 0;
	codec->read_buffer = 0;
	codec->write_buffer = 0;
	codec->chunk = 0;
	codec->buffer_channel = 0; /* Channel of work buffer */
	codec->write_size = 0;          /* Size of work buffer */
	codec->read_size = 0;
	free(codec);
	return 0;
}

void quicktime_init_codec_wmx2(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_wmx2_codec_t));
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_video = 0;
	codec_base->encode_video = 0;
	codec_base->decode_audio = decode;
	codec_base->encode_audio = encode;
	codec_base->fourcc = QUICKTIME_WMX2;
	codec_base->title = "IMA4 on steroids";
	codec_base->desc = "IMA4 on steroids. (Not standardized)";
	codec_base->wav_id = 0x11;
}
