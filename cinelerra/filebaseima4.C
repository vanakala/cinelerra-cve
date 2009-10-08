
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "asset.h"
#include "byteorder.h"
#include "file.h"
#include "filebase.h"
#include "sizes.h"

int FileBase::ima4_step[89] = 
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

int FileBase::ima4_index[16] = 
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};



int FileBase::init_ima4()
{
	ima4_block_samples = 1024;
	ima4_block_size = (ima4_block_samples - 1) * asset->channels / 2 + 4;
	last_ima4_samples = 0;
	last_ima4_indexes = 0;
}

int FileBase::delete_ima4()
{
	if(last_ima4_samples) delete last_ima4_samples;
	if(last_ima4_indexes) delete last_ima4_indexes;
	last_ima4_samples = 0;
	last_ima4_indexes = 0;	
}

int FileBase::ima4_decode_block(int16_t *output, unsigned char *input)
{
return 0;
// 	int predictor[asset->channels];
// 	int index[asset->channels];
// 	int step[asset->channels];
// 	int i, j, nibble;
// 	unsigned char *block_ptr;
// 	unsigned char *input_end = input + ima4_block_size;
// 	int buffer_advance = asset->channels;
// 	int16_t *suboutput;
// 
// // Get the chunk header
// 	for(int i = 0; i < asset->channels; i++)
// 	{
// 		predictor[i] = *input++;
// 		predictor[i] |= (int)(*input++) << 8;
// 		index[i] = *input++;
// 		if(index[i] > 88) index[i] = 88;
// 		if(predictor[i] & 0x8000) predictor[i] -= 0x10000;
// 		step[i] = ima4_step[index[i]];
// 		*output++ = predictor[i];
// 		input++;
// 	}
// 
// // Read the input buffer sequentially, one nibble at a time
// 	while(input < input_end)
// 	{
// 		for(i = 0; i < asset->channels; i++)
// 		{
// 			suboutput = output + i;
// 
// 			for(j = 0; j < 4; j++)
// 			{
// 				ima4_decode_sample(&predictor[i], *input & 0x0f, &index[i], &step[i]);
// 				*suboutput = predictor[i];
// 				suboutput += buffer_advance;
// 				ima4_decode_sample(&predictor[i], (*input++ >> 4) & 0x0f, &index[i], &step[i]);
// 				*suboutput = predictor[i];
// 				suboutput += buffer_advance;
// 			}
// 		}
// 
// 		output += 8 * asset->channels;
// 	}
}

int FileBase::ima4_decode_sample(int *predictor, int nibble, int *index, int *step)
{
	int difference, sign;

// Get new index value
	*index += ima4_index[nibble];

	if(*index < 0) *index = 0; 
	else 
	if(*index > 88) *index = 88;

// Get sign and magnitude from nibble
	sign = nibble & 8;
	nibble = nibble & 7;

// Get difference
	difference = *step >> 3;
	if(nibble & 4) difference += *step;
	if(nibble & 2) difference += *step >> 1;
	if(nibble & 1) difference += *step >> 2;

// Predict value
	if(sign) 
	*predictor -= difference;
	else 
	*predictor += difference;

	if(*predictor > 32767) *predictor = 32767;
	else
	if(*predictor < -32768) *predictor = -32768;

// Update the step value
	*step = ima4_step[*index];

	return 0;
}


int FileBase::ima4_encode_block(unsigned char *output, int16_t *input, int step, int channel)
{
	int i, j, nibble;
	int16_t *input_end = input + ima4_block_size;
	int16_t *subinput;
	int buffer_advance = asset->channels;

	if(!last_ima4_samples)
	{
		last_ima4_samples = new int[asset->channels];
		for(i = 0; i < asset->channels; i++) last_ima4_samples[i] = 0;
	}

	if(!last_ima4_indexes)
	{
		last_ima4_indexes = new int[asset->channels];
		for(i = 0; i < asset->channels; i++) last_ima4_indexes[i] = 0;
	}

	for(i = 0; i < asset->channels; i++)
	{
		*output++ = last_ima4_samples[i] & 0xff;
		*output++ = (last_ima4_samples[i] >> 8) & 0xff;
		*output++ = last_ima4_indexes[i];
		*output++ = 0;
	}

	while(input < input_end)
	{
		for(i = 0; i < asset->channels; i++)
		{
			subinput = input + i;
			for(j = 0; j < 4; j++)
			{
				ima4_encode_sample(&(last_ima4_samples[i]), 
								&(last_ima4_indexes[i]), 
								&nibble, 
								*subinput);

				subinput += buffer_advance;
				*output = nibble;
				
				ima4_encode_sample(&(last_ima4_samples[i]), 
								&(last_ima4_indexes[i]), 
								&nibble, 
								*subinput);

				subinput += buffer_advance;
				*output++ |= (nibble << 4);
			}
		}
		input += 8 * asset->channels;
	}

	return 0;
}

int FileBase::ima4_encode_sample(int *last_sample, int *last_index, int *nibble, int next_sample)
{
	int difference, new_difference, mask, step;

	difference = next_sample - *last_sample;
	*nibble = 0;
	step = ima4_step[*last_index];
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

	*last_index += ima4_index[*nibble];

	if(*last_index < 0) *last_index = 0;
	else
	if(*last_index > 88) *last_index= 88;

	return 0;
}

// Convert the number of samples in a chunk into the number of bytes in that
// chunk.  The number of samples in a chunk should end on a block boundary.
int64_t FileBase::ima4_samples_to_bytes(int64_t samples, int channels)
{
	int64_t bytes = (int64_t)(samples / ima4_block_samples) * ima4_block_size * channels;
	return bytes;
}

int64_t FileBase::ima4_bytes_to_samples(int64_t bytes, int channels)
{
	int64_t samples = (int64_t)(bytes / channels / ima4_block_size) * ima4_block_samples;
	return samples;
}
