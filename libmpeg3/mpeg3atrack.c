#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <stdlib.h>

mpeg3_atrack_t* mpeg3_new_atrack(mpeg3_t *file, 
	int custom_id, 
	int format, 
	mpeg3_demuxer_t *demuxer,
	int number)
{
	mpeg3_atrack_t *new_atrack;

	new_atrack = calloc(1, sizeof(mpeg3_atrack_t));
	if(file->channel_counts)
		new_atrack->channels = file->channel_counts[number];
	new_atrack->sample_rate = 0;
	new_atrack->total_samples = 0;
	new_atrack->demuxer = mpeg3_new_demuxer(file, 1, 0, custom_id);
	if(file->seekable)
	{
		mpeg3demux_copy_titles(new_atrack->demuxer, demuxer);
	}
	new_atrack->current_position = 0;
	new_atrack->pid = custom_id;


/* Copy pointers */
	if(file->sample_offsets)
	{
		new_atrack->sample_offsets = file->sample_offsets[number];
		new_atrack->total_sample_offsets = file->total_sample_offsets[number];
		new_atrack->total_samples = file->total_samples[number];
		new_atrack->demuxer->stream_end = file->audio_eof[number];
	}

	new_atrack->audio = mpeg3audio_new(file, 
		new_atrack, 
		format);

	if(!new_atrack->audio)
	{
/* Failed */
		mpeg3_delete_atrack(file, new_atrack);
		new_atrack = 0;
	}

	return new_atrack;
}

int mpeg3_delete_atrack(mpeg3_t *file, mpeg3_atrack_t *atrack)
{
	if(atrack->audio) mpeg3audio_delete(atrack->audio);
	if(atrack->demuxer) mpeg3_delete_demuxer(atrack->demuxer);
	if(atrack->sample_offsets && atrack->private_offsets)
	{
		free(atrack->sample_offsets);
	}
	free(atrack);
	return 0;
}

void mpeg3_append_samples(mpeg3_atrack_t *atrack, int64_t offset)
{
	if(atrack->total_sample_offsets >= atrack->sample_offsets_allocated)
	{
		atrack->sample_offsets_allocated = 
			MAX(atrack->total_sample_offsets * 2, 1024);
		atrack->sample_offsets = realloc(atrack->sample_offsets,
			sizeof(int64_t) * atrack->sample_offsets_allocated);
	}
	atrack->sample_offsets[atrack->total_sample_offsets++] =
		offset;
	atrack->private_offsets = 1;
}




