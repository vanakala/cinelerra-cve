#include "mpeg3private.h"
#include "mpeg3protos.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>



static pthread_mutex_t *decode_lock = 0;


static void toc_error()
{
	fprintf(stderr, 
		"mpeg3audio: sample accurate seeking without a table of contents \n"
		"is no longer supported.  Use mpeg3toc <mpeg file> <table of contents>\n"
		"to generate a table of contents and load the table of contents instead.\n");
}


static int rewind_audio(mpeg3audio_t *audio)
{
	mpeg3_atrack_t *track = audio->track;
	if(track->sample_offsets)
		mpeg3demux_seek_byte(track->demuxer, track->sample_offsets[0]);
	else
		mpeg3demux_seek_byte(track->demuxer, 0);
	return 0;
}

/* Advance to next header and read it. */
static int read_header(mpeg3audio_t *audio)
{
	int i;
    int try = 0;
	int got_it = 0;
	int result = 0;
	mpeg3_atrack_t *track = audio->track;


	switch(track->format)
	{
		case AUDIO_AC3:
			audio->packet_position = 8;
			result = mpeg3demux_read_data(track->demuxer, 
				audio->packet_buffer + 1, 
				7);


			do
			{
				try++;
				for(i = 0; i < 7; i++)
					audio->packet_buffer[i] = audio->packet_buffer[i + 1];

				if(!result)
				{
					audio->packet_buffer[7] = 
						mpeg3demux_read_char(track->demuxer);
					result = mpeg3demux_eof(track->demuxer);
				}
//printf("read_header 10 %d\n", mpeg3demux_eof(track->demuxer));

				if(!result)
				{
					got_it = (mpeg3_ac3_header(audio->ac3_decoder, 
						audio->packet_buffer) != 0);
				}
				else
					break;

/*
 * printf("read_header 100 offset=%llx got_it=%d\n", 
 * mpeg3demux_tell_byte(track->demuxer),
 * got_it);
 */
			}while(!result && !got_it && try < 0x10000);


			if(!result)
			{
				if(audio->ac3_decoder->channels > track->channels)
					track->channels = audio->ac3_decoder->channels;
				track->sample_rate = audio->ac3_decoder->samplerate;
				audio->framesize = audio->ac3_decoder->framesize;
				if(track->sample_rate <= 0) track->sample_rate = 48000;
//printf("read_header %d %d %d\n", track->channels, track->sample_rate, audio->framesize);
			}
			if(track->sample_rate <= 0) track->sample_rate = 48000;
			break;



		case AUDIO_MPEG:
			audio->packet_position = 4;
/* Layer 1 not supported */
			if(audio->layer_decoder->layer == 1)
			{
				result = 1;
			}

			result = mpeg3demux_read_data(track->demuxer, 
				audio->packet_buffer + 1, 
				3);

			do
			{
				try++;

				for(i = 0; i < 3; i++)
					audio->packet_buffer[i] = audio->packet_buffer[i + 1];

				if(!result)
				{
					audio->packet_buffer[3] = 
							mpeg3demux_read_char(track->demuxer);
					result = mpeg3demux_eof(track->demuxer);
				}

				if(!result)
				{
					got_it = (mpeg3_layer_header(audio->layer_decoder,
						audio->packet_buffer) != 0);
/*
 * printf(__FUNCTION__ " got_it=%d packet=%02x%02x%02x%02x\n",
 * got_it,
 * (unsigned char)audio->packet_buffer[0],
 * (unsigned char)audio->packet_buffer[1],
 * (unsigned char)audio->packet_buffer[2],
 * (unsigned char)audio->packet_buffer[3]);
 */
				}
			}while(!result && !got_it && try < 0x10000);

			if(!result)
			{
				if(audio->layer_decoder->channels > track->channels)
					track->channels = audio->layer_decoder->channels;
				track->sample_rate = audio->layer_decoder->samplerate;
				audio->framesize = audio->layer_decoder->framesize;
			}

			break;

		case AUDIO_PCM:
			audio->packet_position = PCM_HEADERSIZE;
			result = mpeg3demux_read_data(track->demuxer,
				audio->packet_buffer + 1,
				PCM_HEADERSIZE - 1);

			do
			{
				try++;

				for(i = 0; i < PCM_HEADERSIZE - 1; i++)
					audio->packet_buffer[i] = audio->packet_buffer[i + 1];

				if(!result)
				{
					audio->packet_buffer[PCM_HEADERSIZE - 1] = 
							mpeg3demux_read_char(track->demuxer);
					result = mpeg3demux_eof(track->demuxer);
				}

				if(!result)
				{
					got_it = (mpeg3_pcm_header(audio->pcm_decoder,
						audio->packet_buffer) != 0);
				}
			}while(!result && !got_it && try < 0x10000);

			if(!result)
			{
				if(audio->pcm_decoder->channels > track->channels)
					track->channels = audio->pcm_decoder->channels;
				track->sample_rate = audio->pcm_decoder->samplerate;
				audio->framesize = audio->pcm_decoder->framesize;
			}
			break;
	}
	return result;
}


static int delete_struct(mpeg3audio_t *audio)
{
	int i;
	 mpeg3_atrack_t *track = audio->track;

	if(audio->output)
	{
		for(i = 0; i < track->channels; i++)
			free(audio->output[i]);
		free(audio->output);
	}
	if(audio->ac3_decoder) mpeg3_delete_ac3(audio->ac3_decoder);
	if(audio->layer_decoder) mpeg3_delete_layer(audio->layer_decoder);
	if(audio->pcm_decoder) mpeg3_delete_pcm(audio->pcm_decoder);
	free(audio);
	return 0;
}





static int read_frame(mpeg3audio_t *audio, int render)
{
	int result = 0;
	mpeg3_atrack_t *track = audio->track;
	mpeg3_t *file = audio->file;
	float **temp_output = 0;
	int samples = 0;
	int i;
	int old_channels = track->channels;

// Liba52 is not reentrant
	if(track->format == AUDIO_AC3)
	{
		pthread_mutex_lock(decode_lock);
	}

/* Find and read next header */
	result = read_header(audio);


/* Read rest of frame */
	if(!result)
	{
		result = mpeg3demux_read_data(track->demuxer, 
				audio->packet_buffer + audio->packet_position, 
				audio->framesize - audio->packet_position);
	}

/* Handle increase in channel count, for ATSC */
	if(old_channels < track->channels)
	{
		float **new_output = calloc(sizeof(float*), track->channels);
		for(i = 0; i < track->channels; i++)
		{
			new_output[i] = calloc(sizeof(float), audio->output_allocated);
			if(i < old_channels) 
				memcpy(new_output[i], 
					audio->output[i], 
					sizeof(float) * audio->output_size);
		}
		for(i = 0; i < old_channels; i++)
			free(audio->output[i]);
		free(audio->output);
		audio->output = new_output;
	}



	if(render)
	{
		temp_output = malloc(sizeof(float*) * track->channels);
		for(i = 0; i < track->channels; i++)
		{
			temp_output[i] = audio->output[i] + audio->output_size;
		}
	}


	if(!result)
	{
		switch(track->format)
		{
			case AUDIO_AC3:
				samples = mpeg3audio_doac3(audio->ac3_decoder, 
					audio->packet_buffer,
					audio->framesize,
					temp_output,
					render);
//printf("read_frame %d\n", samples);
				break;

			case AUDIO_MPEG:
				switch(audio->layer_decoder->layer)
				{
					case 2:
						samples = mpeg3audio_dolayer2(audio->layer_decoder, 
							audio->packet_buffer,
							audio->framesize,
							temp_output,
							render);

						break;

					case 3:
						samples = mpeg3audio_dolayer3(audio->layer_decoder, 
							audio->packet_buffer,
							audio->framesize,
							temp_output,
							render);
						break;

					default:
						result = 1;
						break;
				}
				break;

			case AUDIO_PCM:
				samples = mpeg3audio_dopcm(audio->pcm_decoder, 
					audio->packet_buffer,
					audio->framesize,
					temp_output,
					render);
				break;
		}
	}


	audio->output_size += samples;
	if(render)
	{
		free(temp_output);
	}

// Liba52 is not reentrant
	if(track->format == AUDIO_AC3)
	{
		pthread_mutex_unlock(decode_lock);
	}


// Shift demuxer data
	if(!file->seekable) 
		mpeg3demux_shift_data(track->demuxer, track->demuxer->data_position);

	return samples;
}






/* Get the length. */
/* Use chunksize if demuxer has a table of contents */
/* For elementary streams use sample count over a certain number of
	bytes to guess total samples */
/* For program streams use timecode */
static int get_length(mpeg3audio_t *audio)
{
	int result = 0;
	mpeg3_t *file = audio->file;
	mpeg3_atrack_t *track = audio->track;
	int samples = 0;

// Table of contents
	if(track->sample_offsets)
	{
		int try = 0;

/* Get stream parameters for header validation */
		while(samples == 0)
		{
			samples = read_frame(audio, 0);
		}

		result = track->total_samples;
	}
	else
// Estimate using multiplexed stream size in seconds
	if(!file->is_audio_stream)
	{
/* Get stream parameters for header validation */
/* Need a table of contents */
		while(samples == 0)
		{
			samples = read_frame(audio, 0);
		}

//		result = (long)(mpeg3demux_length(track->demuxer) * 
//			track->sample_rate);
		result = 0;
	}
	else
// Estimate using average bitrate
	{
		long test_bytes = 0;
		long max_bytes = 0x40000;
		long test_samples = 0;
		int error = 0;
		int64_t total_bytes = mpeg3demux_movie_size(track->demuxer);

		while(!error && test_bytes < max_bytes)
		{
			int samples = read_frame(audio, 0);
			if(!samples) error = 1;
			test_samples += samples;
			test_bytes += audio->framesize;
		}
		result = (long)(((double)total_bytes / test_bytes) * test_samples + 0.5);
	}

	audio->output_size = 0;
	rewind_audio(audio);

	return result;
}





int calculate_format(mpeg3_t *file, mpeg3_atrack_t *track)
{
	int result = 0;

/* Determine the format of the stream.  */
/* If it isn't in the first 8 bytes give up and go to a movie. */
	if(track->format == AUDIO_UNKNOWN)
	{
		unsigned char header[8];
		if(!mpeg3demux_read_data(track->demuxer, 
			header, 
			8))
		{
//printf("calculate_format %lld\n", mpeg3demux_tell_byte(track->demuxer));
			if(!mpeg3_ac3_check(header))
				track->format = AUDIO_AC3;
			else
				track->format = AUDIO_MPEG;
		}
		else
			result = 1;
	}

	return result;
}





mpeg3audio_t* mpeg3audio_new(mpeg3_t *file, 
	mpeg3_atrack_t *track, 
	int format)
{
	mpeg3audio_t *audio = calloc(1, sizeof(mpeg3audio_t));
	int result = 0;
	int i;

	if(!decode_lock)
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		decode_lock = calloc(1, sizeof(pthread_mutex_t));
		pthread_mutex_init(decode_lock, &attr);
	}

	audio->file = file;
	audio->track = track;

	audio->byte_seek = -1;
	audio->sample_seek = -1;
	track->format = format;

	if(file->seekable)
		if(calculate_format(file, track)) result = 1;

//printf("mpeg3audio_new %lld\n", mpeg3demux_tell_byte(track->demuxer));
/* get stream parameters */
	if(!result && file->seekable)
	{
		switch(track->format)
		{
			case AUDIO_AC3:
				audio->ac3_decoder = mpeg3_new_ac3();
				break;
			case AUDIO_MPEG:
				audio->layer_decoder = mpeg3_new_layer();
				break;
			case AUDIO_PCM:
				audio->pcm_decoder = mpeg3_new_pcm();
				break;
		}



		rewind_audio(audio);




		result = read_header(audio);
//printf("mpeg3audio_new 1 %d\n", result);
	}


/* Set up the output buffer */
	if(!result && file->seekable)
	{
		audio->output = calloc(sizeof(float*), track->channels);
		audio->output_allocated = 4;
		for(i = 0; i < track->channels; i++)
		{
			audio->output[i] = calloc(sizeof(float), audio->output_allocated);
		}
	}


/* Calculate Length */
	if(!result && file->seekable)
	{
		rewind_audio(audio);
		track->total_samples = get_length(audio);
	}
	else
	if(file->seekable)
	{
		delete_struct(audio);
		audio = 0;
	}









	return audio;
}







static int seek(mpeg3audio_t *audio)
{
	int result = 0;
	mpeg3_t *file = audio->file;
	mpeg3_atrack_t *track = audio->track;
	mpeg3_demuxer_t *demuxer = track->demuxer;
	int seeked = 0;

/* Stream is unseekable */
	if(!file->seekable) return 0;

/* Sample seek was requested */
	if(audio->sample_seek >= 0)
	{

/*
 * printf(__FUNCTION__ " 1 %d %d %d %d\n", 
 * audio->sample_seek, 
 * track->current_position,
 * audio->output_position, 
 * audio->output_position + audio->output_size);
 */

/* Don't do anything if the destination is inside the sample buffer */
		if(audio->sample_seek >= audio->output_position &&
			audio->sample_seek <= audio->output_position + audio->output_size)
		{
			;
		}
		else
/* Use table of contents */
		if(track->sample_offsets)
		{
			int index;
			int64_t byte;

			index = audio->sample_seek / MPEG3_AUDIO_CHUNKSIZE;
			if(index >= track->total_sample_offsets) index = track->total_sample_offsets - 1;
			byte = track->sample_offsets[index];

			mpeg3demux_seek_byte(demuxer, byte);

			audio->output_position = index * MPEG3_AUDIO_CHUNKSIZE;
			audio->output_size = 0;
			seeked = 1;
		}
		else
		if(!file->is_audio_stream)
/* Use demuxer */
		{
			toc_error();
/*
 * 	   		double time_position = (double)audio->sample_seek / track->sample_rate;
 * 			result |= mpeg3demux_seek_time(demuxer, time_position);
 */
	   		audio->output_position = audio->sample_seek;
			audio->output_size = 0;
			seeked = 1;
		}
		else
/* Use bitrate */
		{
			int64_t byte = (int64_t)((double)audio->sample_seek / 
				track->total_samples * 
				mpeg3demux_movie_size(demuxer));
//printf(__FUNCTION__ " 5\n");

			mpeg3demux_seek_byte(demuxer, byte);
	   		audio->output_position = audio->sample_seek;
			audio->output_size = 0;
			seeked = 1;
		}
	}
	else
/* Percentage seek was requested */
	if(audio->byte_seek >= 0)
	{
		mpeg3demux_seek_byte(demuxer, audio->byte_seek);

// Scan for pts if we're the first to seek.
/*
 * 		if(file->percentage_pts < 0)
 * 		{
 * 			file->percentage_pts = mpeg3demux_scan_pts(demuxer);
 * 		}
 * 		else
 * 		{
 * 			mpeg3demux_goto_pts(demuxer, file->percentage_pts);
 * 		}
 */

	   	audio->output_position = 0;
		audio->output_size = 0;
		seeked = 1;
	}

	if(seeked)
	{
		mpeg3demux_reset_pts(demuxer);
		switch(track->format)
		{
			case AUDIO_MPEG:
				mpeg3_layer_reset(audio->layer_decoder);
				break;
		}
	}
	audio->sample_seek = -1;
	audio->byte_seek = -1;

	return 0;
}










/* ================================================================ */
/*                                    ENTRY POINTS */
/* ================================================================ */



int mpeg3audio_delete(mpeg3audio_t *audio)
{
	delete_struct(audio);
	return 0;
}

int mpeg3audio_seek_byte(mpeg3audio_t *audio, int64_t byte)
{
	audio->byte_seek = byte;
	return 0;
}

int mpeg3audio_seek_sample(mpeg3audio_t *audio, long sample)
{
	mpeg3_atrack_t *track = audio->track;
// Doesn't work for rereading audio during percentage seeking
//	if(sample > track->total_samples) sample = track->total_samples;
	if(sample < 0) sample = 0;
	audio->sample_seek = sample;
	return 0;
}

/* Read raw frames for the mpeg3cat utility */
int mpeg3audio_read_raw(mpeg3audio_t *audio, 
	unsigned char *output, 
	long *size, 
	long max_size)
{
	int result = 0;
	int i;
	mpeg3_atrack_t *track = audio->track;
	*size = 0;


	switch(track->format)
	{
		case AUDIO_AC3:
/* Just write the AC3 stream */
			result = mpeg3demux_read_data(track->demuxer, 
				output, 
				0x800);
			*size = 0x800;
			break;

		case AUDIO_MPEG:
/* Fix the mpeg stream */
			if(!result)
			{
				if(mpeg3demux_read_data(track->demuxer, 
					output, 
					0x800))
					return 1;

				*size += 0x800;
			}
			break;
		
		case AUDIO_PCM:
// This is required to strip the headers
			if(mpeg3demux_read_data(track->demuxer, 
				output, 
				audio->framesize))
				return 1;
			*size = audio->framesize;
			break;
	}
	return result;




#if 0
// This probably doesn't work.
	result = read_header(audio);
	switch(track->format)
	{
		case AUDIO_AC3:
/* Just write the AC3 stream */
			result = mpeg3demux_read_data(track->demuxer, 
				output, 
				audio->framesize);
			*size = audio->framesize;
//printf("mpeg3audio_read_raw 1 %d\n", audio->framesize);
			break;

		case AUDIO_MPEG:
/* Fix the mpeg stream */
			if(!result)
			{
				if(mpeg3demux_read_data(track->demuxer, 
					output, 
					audio->framesize))
					return 1;

				*size += audio->framesize;
			}
			break;
		
		case AUDIO_PCM:
			if(mpeg3demux_read_data(track->demuxer, 
				output, 
				audio->framesize))
				return 1;
			*size = audio->framesize;
			break;
	}
	return result;
#endif

}

void mpeg3_shift_audio(mpeg3audio_t *audio, int diff)
{
	int i, j, k;
	mpeg3_atrack_t *track = audio->track;

	for(k = 0; k < track->channels; k++)
	{
		for(i = 0, j = diff; j < audio->output_size; i++, j++)
		{
			audio->output[k][i] = audio->output[k][j];
		}
	}
	audio->output_size -= diff;
	audio->output_position += diff;
}



/* Channel is 0 to channels - 1 */
int mpeg3audio_decode_audio(mpeg3audio_t *audio, 
		float *output_f, 
		short *output_i, 
		int channel,
		int len)
{
	mpeg3_t *file = audio->file;
	mpeg3_atrack_t *track = audio->track;
	int i, j, k;
	int try = 0;
/* Always render since now the TOC contains index files. */
	int render = 1;
	long new_size;


/* Minimum amount of data must be present for streaming mode */
	if(!file->seekable && 
		track->demuxer->data_size < MPEG3_AUDIO_STREAM_SIZE) return 1;


/* Get header for streaming mode */
	if(track->format == AUDIO_UNKNOWN)
		if(calculate_format(file, track)) return 1;

	if(track->format == AUDIO_AC3 && !audio->ac3_decoder)
		audio->ac3_decoder = mpeg3_new_ac3();
	else
	if(track->format == AUDIO_MPEG && !audio->layer_decoder)
		audio->layer_decoder = mpeg3_new_layer();
	else
	if(track->format == AUDIO_PCM && !audio->pcm_decoder)
		audio->pcm_decoder = mpeg3_new_pcm();



/* Handle seeking requests */
	seek(audio);

	new_size = track->current_position + 
			len + 
			MAXFRAMESAMPLES - 
			audio->output_position;

/* Expand output until enough room exists for new data */
	if(new_size > audio->output_allocated)
	{

		for(i = 0; i < track->channels; i++)
		{
			float *new_output;
			new_output = calloc(sizeof(float), new_size);
			memcpy(new_output, audio->output[i], sizeof(float) * audio->output_size);
			free(audio->output[i]);
			audio->output[i] = new_output;
		}
		audio->output_allocated = new_size;
	}


/* Decode frames until the output is ready */
	while(1)
	{
		if(audio->output_position + audio->output_size >= 
			track->current_position + len ||
			try >= 256 ||
			mpeg3demux_eof(track->demuxer)) break;



		if(!file->seekable && 
			track->demuxer->data_size < 
			MPEG3_AUDIO_STREAM_SIZE) break;

		int samples = read_frame(audio, render);

		if(!samples)
			try++;
		else
			try = 0;
	}




/* Copy the buffer to the output */
	if(channel >= track->channels) channel = track->channels - 1;

	if(output_f)
	{
		for(i = 0, j = track->current_position - audio->output_position; 
			i < len && j < audio->output_size; 
			i++, j++)
		{
			output_f[i] = audio->output[channel][j];
		}
		for( ; i < len; i++)
		{
			output_f[i] = 0;
		}
	}
	else
	if(output_i)
	{
		int sample;
		for(i = 0, j = track->current_position - audio->output_position; 
			i < len && j < audio->output_size; 
			i++, j++)
		{
			sample = (int)(audio->output[channel][j] * 32767);
			if(sample > 32767) sample = 32767;
			else 
			if(sample < -32768) sample = -32768;

			output_i[i] = sample;
		}
		for( ; i < len; i++)
		{
			output_i[i] = 0;
		}
	}



/* Shift audio back */
	if(audio->output_size > MPEG3_AUDIO_HISTORY)
	{
		int diff = audio->output_size - MPEG3_AUDIO_HISTORY;
		mpeg3_shift_audio(audio, diff);
	}



	if(audio->output_size > 0)
		return 0;
	else
		return 1;
}






