#include "faad2/include/faad.h"
#include "funcprotos.h"
#include "quicktime.h"


// Attempts to read more samples than this will crash
#define OUTPUT_ALLOCATION 0x100000
#define CLAMP(x, y, z) ((x) = ((x) <  (y) ? (y) : ((x) > (z) ? (z) : (x))))


typedef struct
{
// Decoder objects
    faacDecHandle decoder_handle;
    faacDecFrameInfo frame_info;
    faacDecConfigurationPtr decoder_config;
	int initialized;
} quicktime_mp4a_codec_t;






static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_mp4a_codec_t *codec = 
		((quicktime_codec_t*)atrack->codec)->priv;
	if(codec->initialized)
	{
		faacDecClose(codec->decoder_handle);
	}

	free(codec);
}

static int decode(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_mp4a_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int64_t current_position = track_map->current_position;
	int64_t end_position = current_position + samples;
	quicktime_vbr_t *vbr = &track_map->vbr;


	if(!codec->initialized)
	{
		unsigned long samplerate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
		unsigned char channels = track_map->channels;
		quicktime_init_vbr(vbr, channels);
		codec->decoder_handle = faacDecOpen();
		codec->decoder_config = faacDecGetCurrentConfiguration(codec->decoder_handle);
		codec->decoder_config->outputFormat = FAAD_FMT_FLOAT;
		codec->decoder_config->defSampleRate = 
			trak->mdia.minf.stbl.stsd.table[0].sample_rate;
		faacDecSetConfiguration(codec->decoder_handle, codec->decoder_config);
		if(faacDecInit(codec->decoder_handle,
			0,
			0,
			&samplerate,
			&channels) < 0)
		{
			return 1;
		}
		codec->initialized = 1;
	}

	if(quicktime_align_vbr(track_map, 
		samples))
	{
		return 1;
	}
	else
	{
// Decode until buffer is full
		while(quicktime_vbr_end(vbr) < end_position)
		{
			if(!quicktime_read_vbr(file, track_map))
			{
				faacDecFrameInfo frame_info;
				float *sample_buffer = faacDecDecode(codec->decoder_handle, 
					&frame_info,
            		quicktime_vbr_input(vbr), 
					quicktime_vbr_input_size(vbr));
/*
 * static FILE *test = 0;
 * if(!test) test = fopen("/hmov/test.pcm", "w");
 * int i;
 * for(i = 0; i < frame_info.samples; i++)
 * {
 * int16_t sample = (int)(sample_buffer[i] * 32767);
 * fwrite(&sample, 2, 1, test);
 * }
 * fflush(test);
 */

				quicktime_store_vbr_float(track_map,
					sample_buffer,
					frame_info.samples / track_map->channels);
			}
			else
			{
				break;
			}
		}

// Transfer from buffer to output
		if(output_i)
			quicktime_copy_vbr_int16(vbr, 
				current_position, 
				samples, 
				output_i, 
				channel);
		else
		if(output_f)
			quicktime_copy_vbr_float(vbr, 
				current_position, 
				samples,
				output_f, 
				channel);
	}
	return 0;
}




void quicktime_init_codec_mp4a(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_mp4a_codec_t *codec;
	codec_base->priv = calloc(1, sizeof(quicktime_mp4a_codec_t));
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_audio = decode;
	codec_base->fourcc = "mp4a";
	codec_base->title = "Advanced Audio Codec";
	codec_base->desc = "Audio section of MPEG4 standard";

	codec = (quicktime_mp4a_codec_t*)codec_base->priv;
// Default encoding parameters here
}




