#include "audiodevice.h"
#include "condition.h"
#include "dcoffset.h"
#include "bcprogressbox.h"


int AudioDevice::set_record_dither(int bits)
{
	rec_dither = bits;
	return 0;
}


int AudioDevice::get_dc_offset(int *output, RecordGUIDCOffsetText **dc_offset_text)
{
	dc_offset_thread->calibrate_dc_offset(output, dc_offset_text, get_ichannels());
	return 0;
}

int AudioDevice::set_dc_offset(int dc_offset, int channel)
{
	dc_offset_thread->dc_offset[channel] = dc_offset;
	return 0;
}

#define GET_PEAK_MACRO \
					input_channel[j] = sample;                          \
					if(sample > max[i]) max[i] = sample;           \
					else if(-sample > max[i]) max[i] = -sample;

// ============================ use 2 macros to allow for getting dc_offset
// ============= must check for overload after dc_offset because of byte wrapping on save

#define GET_8BIT_SAMPLE_MACRO1 \
sample = input_buffer[k];      \
sample -= dc_offset_value;        \
k += input_channels;           \
if(sample >= max_sample[i]) { sample = max_sample[i]; if(over_count < 3) over_count++; } \
else                           \
if(sample <= min_sample[i]) { sample = min_sample[i]; if(over_count < 3) over_count++; } \
else                           \
if(over_count < 3) over_count = 0; 

#define GET_8BIT_SAMPLE_MACRO2 \
sample /= 0x7f;                  



#define GET_16BIT_SAMPLE_MACRO1                            \
sample = input_buffer_16[k];                               \
if(dither_scale) { dither_value = rand() % dither_scale; sample -= dither_value; } \
sample -= dc_offset_value;                                    \
k += input_channels;                                       \
if(sample >= max_sample[i]) { sample = max_sample[i]; if(over_count < 3) over_count++; } \
else                                                       \
if(sample <= min_sample[i]) { sample = min_sample[i]; if(over_count < 3) over_count++; } \
else                                                       \
if(over_count < 3) over_count = 0;                         \

#define GET_16BIT_SAMPLE_MACRO2                            \
sample /= 0x7fff;                  



#define GET_24BIT_SAMPLE_MACRO1 \
sample = (unsigned char)input_buffer[k] | \
			(((unsigned char)input_buffer[k + 1]) << 8) | \
			(((int)input_buffer[k + 2]) << 16); \
sample -= dc_offset_value; \
k += input_channels * 3; \
if(sample >= max_sample[i]) { sample = max_sample[i]; if(over_count < 3) over_count++; } \
else                                                 \
if(sample <= min_sample[i]) { sample = min_sample[i]; if(over_count < 3) over_count++; } \
else                                                 \
if(over_count < 3) over_count = 0; 

#define GET_24BIT_SAMPLE_MACRO2       \
sample /= 0x7fffff;



#define GET_32BIT_SAMPLE_MACRO1 \
sample = (unsigned char)input_buffer[k] | \
			(((unsigned char)input_buffer[k + 1]) << 8) | \
			(((unsigned char)input_buffer[k + 2]) << 16) | \
			(((int)input_buffer[k + 3]) << 24); \
sample -= dc_offset_value; \
k += input_channels * 4; \
if(sample >= max_sample[i]) { sample = max_sample[i]; if(over_count < 3) over_count++; } \
else \
if(sample <= min_sample[i]) { sample = min_sample[i]; if(over_count < 3) over_count++; } \
else \
if(over_count < 3) over_count = 0; 

#define GET_32BIT_SAMPLE_MACRO2       \
sample /= 0x7fffffff;

int AudioDevice::read_buffer(double **input, 
	int samples, 
	int channels, 
	int *over, 
	double *max, 
	int input_offset)
{
	int i, j, k, frame, bits;
	double sample, denominator;
	double min_sample[MAXCHANNELS], max_sample[MAXCHANNELS];
	int sample_int;
	int over_count;
	int dither_value, dither_scale;
	int input_channels;
	int result = 0;
	double *input_channel;
	int *dc_offset_total;
	int dc_offset_value;

	is_recording = 1;
	record_timer.update();

	bits = get_ibits();
	input_channels = get_ichannels();
	frame = input_channels * bits / 8;

	dither_scale = 0;
	total_samples_read += samples;

	switch(bits)
	{
		case 8:       
			denominator = 0x7f;          
			break;
		case 16:      
			denominator = 0x7fff;        
			if(rec_dither == 8)
			{
				dither_scale = 255;
			}
			break;
		case 24:      
			denominator = 0x7fffff;      
			if(rec_dither == 8)
			{
				dither_scale = 65535;
			}
			else 
			if (rec_dither == 16)
			{
				dither_scale = 255;
			}
			break;
		case 32:      
			denominator = 0x7fffffff;      
			dither_scale = 0;
			break;
	}

	if(input_buffer == 0) input_buffer = new char[samples * frame];

	if(duplex_init && !record_before_play)
	{
// block until playback starts
		duplex_lock->lock("AudioDevice::read_buffer");
		duplex_init = 0;
	}

	result = get_lowlevel_in()->read_buffer(input_buffer, samples * frame);

// allow playback to start
	if(duplex_init && record_before_play)
	{
		duplex_lock->unlock();
		duplex_init = 0;
	}


	if(result < 0)
	{
		perror("AudioDevice::read_buffer");
		sleep(1);
	}

	for(i = 0; i < channels && i < get_ichannels(); i++)
	{
		input_channel = &input[i][input_offset];
		dc_offset_value = dc_offset_thread->dc_offset[i];

// calculate minimum and maximum samples
		if(dc_offset_thread->dc_offset[i] <= 0) 
		{ 
			min_sample[i] = -denominator - dc_offset_thread->dc_offset[i]; 
			max_sample[i] = denominator; 
		}
		else 
		{ 
			min_sample[i] = -denominator; 
			max_sample[i] = denominator - dc_offset_thread->dc_offset[i]; 
		}
		max[i] = 0; 
		over_count = 0;

// device is set to little endian
		switch(bits)
		{
			case 8:
				if(dc_offset_thread->getting_dc_offset)
				{
					dc_offset_total = &(dc_offset_thread->dc_offset_total[i]);
					for(j = 0, k = i; j < samples; j++)
					{
						GET_8BIT_SAMPLE_MACRO1
						(*dc_offset_total) += (int)sample;
						GET_8BIT_SAMPLE_MACRO2
						GET_PEAK_MACRO
					}
				}
				else
				{
					for(j = 0, k = i; j < samples; j++)
					{
						GET_8BIT_SAMPLE_MACRO1
						GET_8BIT_SAMPLE_MACRO2
						GET_PEAK_MACRO
					}
				}
				break;
				
			case 16:
				{
					int16_t *input_buffer_16;
					input_buffer_16 = (int16_t *)input_buffer;
					dc_offset_total = &(dc_offset_thread->dc_offset_total[i]);
					
					if(dc_offset_thread->getting_dc_offset)
					{
						for(j = 0, k = i; j < samples; j++)
						{
							GET_16BIT_SAMPLE_MACRO1
							(*dc_offset_total) += (int)sample;
							GET_16BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}
					}
					else
					{
						for(j = 0, k = i; j < samples; j++)
						{
							GET_16BIT_SAMPLE_MACRO1
							GET_16BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}
					}
				}
				break;
				
			case 24:
				{
					dc_offset_total = &(dc_offset_thread->dc_offset_total[i]);
					
					if(dc_offset_thread->getting_dc_offset)
					{
						for(j = 0, k = i * 3; j < samples; j++)
						{
							GET_24BIT_SAMPLE_MACRO1
							(*dc_offset_total) += (int)sample;
							GET_24BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}
					}
					else
					{
						for(j = 0, k = i * 3; j < samples; j++)
						{
							GET_24BIT_SAMPLE_MACRO1
							GET_24BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}
					}
				}
				
			case 32:
				{
					dc_offset_total = &(dc_offset_thread->dc_offset_total[i]);
					
					if(dc_offset_thread->getting_dc_offset)
					{
						for(j = 0, k = i * 4; j < samples; j++)
						{
							GET_32BIT_SAMPLE_MACRO1
							(*dc_offset_total) += (int)sample;
							GET_32BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}
					}
					else
					{
						for(j = 0, k = i * 4; j < samples; j++)
						{
							GET_32BIT_SAMPLE_MACRO1
							GET_32BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}
					}
				}
				break;
		}
		if(over_count >= 3) over[i] = 1; else over[i] = 0;
	}

	if(dc_offset_thread->getting_dc_offset) 
	{
		dc_offset_thread->dc_offset_count += samples * channels;
		if(dc_offset_thread->progress->update(dc_offset_thread->dc_offset_count, 1))
		{
			dc_offset_thread->getting_dc_offset = 0;
			dc_offset_thread->dc_offset_lock->unlock();
		}
		else
		if(dc_offset_thread->dc_offset_count > 256000)
		{
			for(i = 0; i < get_ichannels(); i++)
			{
				dc_offset_thread->dc_offset[i] = dc_offset_thread->dc_offset_total[i] / dc_offset_thread->dc_offset_count * 2; // don't know why * 2
			}
			dc_offset_thread->getting_dc_offset = 0;
			dc_offset_thread->dc_offset_lock->unlock();
		}
	}
	return result < 0;
}
