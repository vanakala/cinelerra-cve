#include "funcprotos.h"
#include "lame.h"
#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "quicktime.h"
#include "qtmp3.h"

#define CLAMP(x, y, z) ((x) = ((x) <  (y) ? (y) : ((x) > (z) ? (z) : (x))))

#define OUTPUT_ALLOCATION 0x100000

//static FILE *test = 0;

typedef struct
{
// mp3 decoder
	mpeg3_layer_t *mp3;
// Can't use same structure for header testing
	mpeg3_layer_t *mp3_header;
	unsigned char *packet_buffer;
	int packet_allocated;


// Number of first sample in output relative to file
	int64_t output_position;
// Number of samples in output buffer
	long output_size;
// Number of samples allocated in output buffer
	long output_allocated;
// Current reading position in file
	int64_t chunk;
	int decode_initialized;
	float **output;



// mp3 encoder
	lame_global_flags *lame_global;
// This calculates the number of samples per chunk
	mpeg3_layer_t *encoded_header;
	int encode_initialized;
	float **input;
	int input_size;
	int input_allocated;
	int bitrate;
	unsigned char *encoder_output;
	int encoder_output_size;
	int encoder_output_allocated;
} quicktime_mp3_codec_t;





static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
	if(codec->mp3) mpeg3_delete_layer(codec->mp3);
	if(codec->mp3_header)  mpeg3_delete_layer(codec->mp3_header);
	if(codec->packet_buffer) free(codec->packet_buffer);
	if(codec->output)
	{
		int i;
		for(i = 0; i < atrack->channels; i++)
			free(codec->output[i]);
		free(codec->output);
	}

	if(codec->lame_global)
	{
		lame_close(codec->lame_global);
	}

	if(codec->input)
	{
		int i;
		for(i = 0; i < atrack->channels; i++)
		{
			free(codec->input[i]);
		}
		free(codec->input);
	}

	if(codec->encoder_output)
		free(codec->encoder_output);

	if(codec->encoded_header)
		mpeg3_delete_layer(codec->encoded_header);

	free(codec);

	return 0;
}

static int chunk_len(quicktime_t *file,
	quicktime_mp3_codec_t *codec,
	int64_t offset,
	int64_t next_chunk)
{
	int result = 0;
	unsigned char header[4];
	int accum = 0;

	while(offset < next_chunk)
	{
		quicktime_set_position(file, offset);
		result = !quicktime_read_data(file, (unsigned char*)&header, 4);

		if(result)
		{
			return accum;
		}

// Decode size of mp3 frame
		result = mpeg3_layer_header(codec->mp3_header,
			header);

// Invalid header
		if(!result) 
			return accum;
		else
// Valid header
		{
			accum += result;
			offset += result;
			quicktime_set_position(file, offset + result);
		}
	}
	return accum;
}










static int decode(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	int result = 0;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_trak_t *trak = track_map->track;
	long current_position = track_map->current_position;
	long end_position = current_position + samples;
	float *pcm;
	int i, j, k;
	int64_t offset1;
	int64_t offset2;
	int chunk_size;
	int new_size;
	int frame_size;
	int try = 0;
	float **temp_output;

	if(samples > OUTPUT_ALLOCATION)
		printf("decode: can't read more than %d samples at a time.\n", OUTPUT_ALLOCATION);


//printf("decode 1\n");
	if(output_i) bzero(output_i, sizeof(int16_t) * samples);
	if(output_f) bzero(output_f, sizeof(float) * samples);

	temp_output = malloc(sizeof(float*) * track_map->channels);

// Seeked outside output buffer's range or not initialized: restart
	if(current_position < codec->output_position ||
		current_position > codec->output_position + codec->output_size ||
		!codec->decode_initialized)
	{
		quicktime_chunk_of_sample(&codec->output_position, 
			&codec->chunk, 
			trak, 
			current_position);

// We know the first mp3 packet in the chunk has a pcm_offset from the encoding.
		codec->output_size = 0;
//printf("decode 1 %lld %d\n", codec->output_position, quicktime_chunk_samples(trak, codec->chunk));
		codec->output_position = quicktime_sample_of_chunk(trak, codec->chunk);
//printf("decode 2 %lld\n", codec->output_position);

// Initialize and load initial buffer for decoding
		if(!codec->decode_initialized)
		{
			int i;
			codec->decode_initialized = 1;
			codec->output = malloc(sizeof(float*) * track_map->channels);
			for(i = 0; i < track_map->channels; i++)
			{
				codec->output[i] = malloc(sizeof(float) * OUTPUT_ALLOCATION);
			}
			codec->output_allocated = OUTPUT_ALLOCATION;
			codec->mp3 = mpeg3_new_layer();
			codec->mp3_header = mpeg3_new_layer();
		}
	}

// Decode chunks until output is big enough
	while(codec->output_position + codec->output_size < 
		current_position + samples &&
		try < 6)
	{
// Decode a chunk
		offset1 = quicktime_chunk_to_offset(file, trak, codec->chunk);
		offset2 = quicktime_chunk_to_offset(file, trak, codec->chunk + 1);

		if(offset2 == offset1) break;

		chunk_size = chunk_len(file, codec, offset1, offset2);

		if(codec->packet_allocated < chunk_size && 
			codec->packet_buffer)
		{
			free(codec->packet_buffer);
			codec->packet_buffer = 0;
		}

		if(!codec->packet_buffer)
		{
			codec->packet_buffer = calloc(1, chunk_size);
			codec->packet_allocated = chunk_size;
		}

		quicktime_set_position(file, offset1);
		result = !quicktime_read_data(file, codec->packet_buffer, chunk_size);
		if(result) break;

		for(i = 0; i < chunk_size; )
		{
// Allocate more output
			new_size = codec->output_size + MAXFRAMESAMPLES;
			if(new_size > codec->output_allocated)
			{
				for(j = 0; j < track_map->channels; j++)
				{
					float *new_output = calloc(sizeof(float), new_size);
					memcpy(new_output, 
						codec->output[j], 
						sizeof(float) * codec->output_size);
					free(codec->output[j]);
					codec->output[j] = new_output;
				}
				codec->output_allocated = new_size;
			}

// Decode a frame
			for(j = 0; j < track_map->channels; j++)
				temp_output[j] = codec->output[j] + codec->output_size;

			frame_size = mpeg3_layer_header(codec->mp3, 
				codec->packet_buffer + i);

			result = mpeg3audio_dolayer3(codec->mp3, 
				codec->packet_buffer + i, 
				frame_size, 
				temp_output, 
				1);


			if(result)
			{
				codec->output_size += result;
				result = 0;
				try = 0;
			}
			else
			{
				try++;
			}

			i += frame_size;
		}

		codec->chunk++;
	}

// Transfer region of output to argument
	pcm = codec->output[channel];
	if(output_i)
	{
		for(i = current_position - codec->output_position, j = 0; 
			j < samples && i < codec->output_size; 
			j++, i++)
		{
			int sample = pcm[i] * 32767;
			CLAMP(sample, -32768, 32767);
			output_i[j] = sample;
		}
	}
	else
	if(output_f)
	{
		for(i = current_position - codec->output_position, j = 0; 
			j < samples && i < codec->output_size; 
			j++, i++)
		{
			output_f[j] = pcm[i];
		}
	}

// Delete excess output
	if(codec->output_size > OUTPUT_ALLOCATION)
	{
		int diff = codec->output_size - OUTPUT_ALLOCATION;
		for(k = 0; k < track_map->channels; k++)
		{
			pcm = codec->output[k];
			for(i = 0, j = diff; j < codec->output_size; i++, j++)
			{
				pcm[i] = pcm[j];
			}
		}
		codec->output_size -= diff;
		codec->output_position += diff;
	}

	free(temp_output);
//printf("decode 100\n");
	return 0;
}




static int allocate_output(quicktime_mp3_codec_t *codec,
	int samples)
{
	int new_size = codec->encoder_output_size + samples * 4;
	if(codec->encoder_output_allocated < new_size)
	{
		unsigned char *new_output = calloc(1, new_size);

		if(codec->encoder_output)
		{
			memcpy(new_output, 
				codec->encoder_output, 
				codec->encoder_output_size);
			free(codec->encoder_output);
		}
		codec->encoder_output = new_output;
		codec->encoder_output_allocated = new_size;
	}

}



// Empty the output buffer of frames
static int write_frames(quicktime_t *file, 
	quicktime_audio_map_t *track_map,
	quicktime_trak_t *trak,
	quicktime_mp3_codec_t *codec)
{
	int result = 0;
	int i, j;
	int frames_end = 0;
	quicktime_atom_t chunk_atom;

// Write to chunks
	for(i = 0; i < codec->encoder_output_size - 4; )
	{
		unsigned char *header = codec->encoder_output + i;
		int frame_size = mpeg3_layer_header(codec->encoded_header, header);

		if(frame_size)
		{
// Frame is finished before end of buffer
			if(i + frame_size <= codec->encoder_output_size)
			{
// Write the chunk
				int64_t offset;
				int frame_samples = mpeg3audio_dolayer3(codec->encoded_header, 
					header, 
					frame_size, 
					0,
					0);

				quicktime_write_chunk_header(file, trak, &chunk_atom);
				result = !quicktime_write_data(file, header, frame_size);
// Knows not to save the chunksizes for audio
				quicktime_write_chunk_footer(file, 
					trak, 
					track_map->current_chunk,
					&chunk_atom, 
					frame_samples);

				track_map->current_chunk++;
				i += frame_size;
				frames_end = i;
			}
			else
// Frame isn't finished before end of buffer.
			{
				frames_end = i;
				break;
			}
		}
		else
// Not the start of a frame.  Skip it.
		{
			i++;
			frames_end = i;
		}
	}

	if(frames_end > 0)
	{
		for(i = frames_end, j = 0; i < codec->encoder_output_size; i++, j++)
		{
			codec->encoder_output[j] = codec->encoder_output[i];
		}
		codec->encoder_output_size -= frames_end;
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
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int new_size = codec->input_size + samples;
	int i, j;
	int frames_end = 0;

	if(!codec->encode_initialized)
	{
		codec->encode_initialized = 1;
		codec->lame_global = lame_init();
		lame_set_brate(codec->lame_global, codec->bitrate / 1000);
		lame_set_quality(codec->lame_global, 0);
		lame_set_in_samplerate(codec->lame_global, 
			trak->mdia.minf.stbl.stsd.table[0].sample_rate);
		if((result = lame_init_params(codec->lame_global)) < 0)
			printf("encode: lame_init_params returned %d\n", result);
		codec->encoded_header = mpeg3_new_layer();
		if(file->use_avi)
			trak->mdia.minf.stbl.stsd.table[0].sample_size = 0;
	}


// Stack input on end of buffer
	if(new_size > codec->input_allocated)
	{
		float *new_input;

		if(!codec->input) 
			codec->input = calloc(sizeof(float*), track_map->channels);

		for(i = 0; i < track_map->channels; i++)
		{
			new_input = calloc(sizeof(float), new_size);
			if(codec->input[i])
			{
				memcpy(new_input, codec->input[i], sizeof(float) * codec->input_size);
				free(codec->input[i]);
			}
			codec->input[i] = new_input;
		}
		codec->input_allocated = new_size;
	}


// Transfer to input buffers
	if(input_i)
	{
		for(i = 0; i < track_map->channels; i++)
		{
			for(j = 0; j < samples; j++)
				codec->input[i][j] = input_i[i][j];
		}
	}
	else
	if(input_f)
	{
		for(i = 0; i < track_map->channels; i++)
		{
			for(j = 0; j < samples; j++)
				codec->input[i][j] = input_f[i][j] * 32767;
		}
	}

// Encode
	allocate_output(codec, samples);

	result = lame_encode_buffer_float(codec->lame_global,
		codec->input[0],
		(track_map->channels > 1) ? codec->input[1] : codec->input[0],
		samples,
		codec->encoder_output + codec->encoder_output_size,
		codec->encoder_output_allocated - codec->encoder_output_size);

	codec->encoder_output_size += result;

	result = write_frames(file,
		track_map,
		trak,
		codec);

	return result;
}



static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(!strcasecmp(key, "mp3_bitrate"))
		codec->bitrate = *(int*)value;

	return 0;
}



static void flush(quicktime_t *file, int track)
{
	int result = 0;
	int64_t offset = quicktime_position(file);
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

	if(codec->encode_initialized)
	{
		result = lame_encode_flush(codec->lame_global,
        	codec->encoder_output + codec->encoder_output_size, 
			codec->encoder_output_allocated - codec->encoder_output_size);
		codec->encoder_output_size += result;
		result = write_frames(file, 
			track_map,
			trak,
			codec);
	}
}


void quicktime_init_codec_mp3(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_mp3_codec_t *codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_mp3_codec_t));
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_audio = decode;
	codec_base->encode_audio = encode;
	codec_base->set_parameter = set_parameter;
	codec_base->flush = flush;
	codec_base->fourcc = QUICKTIME_MP3;
	codec_base->title = "MP3";
	codec_base->desc = "MP3 for video";
	codec_base->wav_id = 0x55;

	codec = codec_base->priv;
	codec->bitrate = 256000;
}




