#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"

static int delete_vcodec_stub(quicktime_video_map_t *vtrack)
{
	printf("delete_vcodec_stub called\n");
	return 0;
}

static int delete_acodec_stub(quicktime_audio_map_t *atrack)
{
	printf("delete_acodec_stub called\n");
	return 0;
}

static int decode_video_stub(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track)
{
	printf("decode_video_stub called\n");
	return 1;
}

static int encode_video_stub(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track)
{
	printf("encode_video_stub called\n");
	return 1;
}

static int decode_audio_stub(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	printf("decode_audio_stub called\n");
	return 1;
}

static int encode_audio_stub(quicktime_t *file, 
				int16_t **input_i, 
				float **input_f, 
				int track, 
				long samples)
{
	printf("encode_audio_stub called\n");
	return 1;
}


static int reads_colormodel_stub(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888);
}

static int writes_colormodel_stub(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888);
}

static void flush_codec_stub(quicktime_t *file, int track)
{
}

/* Convert Microsoft audio id to codec */
void quicktime_id_to_codec(char *result, int id)
{
	switch(id)
	{
		case 0x55:
			memcpy(result, QUICKTIME_MP3, 4);
			break;
		case 0x161:
			memcpy(result, QUICKTIME_WMA, 4);
			break;
		default:
			printf("quicktime_id_to_codec: unknown audio id: %p\n", id);
			break;
	}
}

int quicktime_codec_to_id(char *codec)
{
	if(quicktime_match_32(codec, QUICKTIME_MP3))
		return 0x55;
	else
	if(quicktime_match_32(codec, QUICKTIME_WMA))
		return 0x161;
	else
		printf("quicktime_codec_to_id: unknown codec %c%c%c%c\n", codec[0], codec[1], codec[2], codec[3]);
}


static quicktime_codec_t* new_codec()
{
	quicktime_codec_t *codec = calloc(1, sizeof(quicktime_codec_t));
	codec->delete_vcodec = delete_vcodec_stub;
	codec->delete_acodec = delete_acodec_stub;
	codec->decode_video = decode_video_stub;
	codec->encode_video = encode_video_stub;
	codec->decode_audio = decode_audio_stub;
	codec->encode_audio = encode_audio_stub;
	codec->reads_colormodel = reads_colormodel_stub;
	codec->writes_colormodel = writes_colormodel_stub;
	codec->flush = flush_codec_stub;
	return codec;
}

int new_vcodec(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = vtrack->codec = new_codec();
	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	int result = quicktime_find_vcodec(vtrack);

	if(result)
	{
		fprintf(stderr, 
			"new_vcodec: couldn't find codec for \"%c%c%c%c\"\n",
			compressor[0],
			compressor[1],
			compressor[2],
			compressor[3]);
		free(codec_base);
		vtrack->codec = 0;
		return 1;
	}

	return 0;
}

int new_acodec(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = atrack->codec = new_codec();
	char *compressor = atrack->track->mdia.minf.stbl.stsd.table[0].format;
	int result = quicktime_find_acodec(atrack);

	if(result)
	{
		fprintf(stderr, 
			"new_acodec: couldn't find codec for \"%c%c%c%c\"\n",
			compressor[0],
			compressor[1],
			compressor[2],
			compressor[3]);
		free(codec_base);
		atrack->codec = 0;
		return 1;
	}

	return 0;
}

int quicktime_init_vcodec(quicktime_video_map_t *vtrack)
{
	int result = new_vcodec(vtrack);
	return result;
}

int quicktime_init_acodec(quicktime_audio_map_t *atrack)
{
	int result = new_acodec(atrack);
	return result;
}


int quicktime_delete_vcodec(quicktime_video_map_t *vtrack)
{
	if(vtrack->codec)
	{
		quicktime_codec_t *codec_base = vtrack->codec;
		if(codec_base->priv)
			codec_base->delete_vcodec(vtrack);
		free(vtrack->codec);
	}
	vtrack->codec = 0;
	return 0;
}

int quicktime_delete_acodec(quicktime_audio_map_t *atrack)
{
	if(atrack->codec)
	{
		quicktime_codec_t *codec_base = atrack->codec;
		if(codec_base->priv)
			codec_base->delete_acodec(atrack);
		free(atrack->codec);
	}
	atrack->codec = 0;
	return 0;
}

int quicktime_supported_video(quicktime_t *file, int track)
{
	if(track < file->total_vtracks)
	{
		quicktime_video_map_t *video_map = &file->vtracks[track];

		if(video_map->codec)
			return 1;
		else
			return 0;
	}
	return 0;
}

int quicktime_supported_audio(quicktime_t *file, int track)
{
	if(track < file->total_atracks)
	{
		quicktime_audio_map_t *audio_map = &file->atracks[track];
		if(audio_map->codec)
			return 1;
		else
			return 0;
	}
	return 0;
	
}


long quicktime_decode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track)
{
	int result;

	if(track < 0 || track >= file->total_vtracks)
	{
		fprintf(stderr, "quicktime_decode_video: track %d out of range %d - %d\n",
			track,
			0,
			file->total_vtracks);
		return 1;
	}

/* Get dimensions from first video track */
	if(!file->do_scaling)
	{
		quicktime_video_map_t *video_map = &file->vtracks[track];
		quicktime_trak_t *trak = video_map->track;
		int track_width = trak->tkhd.track_width;
		int track_height = trak->tkhd.track_height;

		file->in_x = 0;
		file->in_y = 0;
		file->in_w = track_width;
		file->in_h = track_height;
		file->out_w = track_width;
		file->out_h = track_height;
	}

	result = ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file, 
		row_pointers, 
		track);
	file->vtracks[track].current_position++;
	return result;
}

void quicktime_set_parameter(quicktime_t *file, char *key, void *value)
{
	int i;
	for(i = 0; i < file->total_vtracks; i++)
	{
		quicktime_codec_t *codec = (quicktime_codec_t*)file->vtracks[i].codec;
		if(codec)
			if(codec->set_parameter) codec->set_parameter(file, i, key, value);
	}

	for(i = 0; i < file->total_atracks; i++)
	{
		quicktime_codec_t *codec = (quicktime_codec_t*)file->atracks[i].codec;
		if(codec)
			if(codec->set_parameter) codec->set_parameter(file, i, key, value);
	}
}

int quicktime_encode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track)
{
	int result;
	result = ((quicktime_codec_t*)file->vtracks[track].codec)->encode_video(file, row_pointers, track);
	file->vtracks[track].current_position++;
	return result;
}


int quicktime_decode_audio(quicktime_t *file, 
				int16_t *output_i, 
				float *output_f, 
				long samples, 
				int channel)
{
	int quicktime_track, quicktime_channel;
	int result = 1;

	quicktime_channel_location(file, &quicktime_track, &quicktime_channel, channel);
	result = ((quicktime_codec_t*)file->atracks[quicktime_track].codec)->decode_audio(file, 
				output_i, 
				output_f, 
				samples, 
				quicktime_track, 
				quicktime_channel);
	file->atracks[quicktime_track].current_position += samples;

	return result;
}

/* Since all channels are written at the same time: */
/* Encode using the compressor for the first audio track. */
/* Which means all the audio channels must be on the same track. */

int quicktime_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, long samples)
{
	int result = 1;
	char *compressor = quicktime_audio_compressor(file, 0);

	result = ((quicktime_codec_t*)file->atracks[0].codec)->encode_audio(file, 
		input_i, 
		input_f,
		0, 
		samples);
	file->atracks[0].current_position += samples;

	return result;
}

int quicktime_reads_cmodel(quicktime_t *file, 
	int colormodel, 
	int track)
{
	int result = ((quicktime_codec_t*)file->vtracks[track].codec)->reads_colormodel(file, colormodel, track);
	return result;
}

int quicktime_writes_cmodel(quicktime_t *file, 
	int colormodel, 
	int track)
{
	return ((quicktime_codec_t*)file->vtracks[track].codec)->writes_colormodel(file, colormodel, track);
}

/* Compressors that can only encode a window at a time */
/* need to flush extra data here. */

int quicktime_flush_acodec(quicktime_t *file, int track)
{
	((quicktime_codec_t*)file->atracks[track].codec)->flush(file, track);
	return 0;
};

void quicktime_flush_vcodec(quicktime_t *file, int track)
{
	((quicktime_codec_t*)file->vtracks[track].codec)->flush(file, track);
}

int64_t quicktime_samples_to_bytes(quicktime_trak_t *track, long samples)
{
	char *compressor = track->mdia.minf.stbl.stsd.table[0].format;
	int channels = track->mdia.minf.stbl.stsd.table[0].channels;

	if(quicktime_match_32(compressor, QUICKTIME_IMA4)) 
		return samples * channels;

	if(quicktime_match_32(compressor, QUICKTIME_ULAW)) 
		return samples * channels;

/* Default use the sample size specification for TWOS and RAW */
	return samples * channels * track->mdia.minf.stbl.stsd.table[0].sample_size / 8;
}

int quicktime_codecs_flush(quicktime_t *file)
{
	int result = 0;
	int i;
	if(!file->wr) return result;

	if(file->total_atracks)
	{
		for(i = 0; i < file->total_atracks && !result; i++)
		{
			quicktime_flush_acodec(file, i);
		}
	}

	if(file->total_vtracks)
	{
		for(i = 0; i < file->total_vtracks && !result; i++)
		{
			quicktime_flush_vcodec(file, i);
		}
	}
	return result;
}
