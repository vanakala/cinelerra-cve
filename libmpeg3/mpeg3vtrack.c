#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <stdlib.h>

mpeg3_vtrack_t* mpeg3_new_vtrack(mpeg3_t *file, 
	int custom_id, 
	mpeg3_demuxer_t *demuxer,
	int number)
{
	int result = 0;
	mpeg3_vtrack_t *new_vtrack;

	new_vtrack = calloc(1, sizeof(mpeg3_vtrack_t));
	new_vtrack->demuxer = mpeg3_new_demuxer(file, 0, 1, custom_id);
	new_vtrack->frame_cache = mpeg3_new_cache();
	if(file->seekable)
	{
		mpeg3demux_copy_titles(new_vtrack->demuxer, demuxer);
	}
	new_vtrack->current_position = 0;
	new_vtrack->pid = custom_id;

// Copy pointers
	if(file->frame_offsets)
	{
		new_vtrack->frame_offsets = file->frame_offsets[number];
		new_vtrack->total_frame_offsets = file->total_frame_offsets[number];
		new_vtrack->keyframe_numbers = file->keyframe_numbers[number];
		new_vtrack->total_keyframe_numbers = file->total_keyframe_numbers[number];
		new_vtrack->demuxer->stream_end = file->video_eof[number];
	}

/* Get information about the track here. */
	new_vtrack->video = mpeg3video_new(file, 
		new_vtrack);

	if(!new_vtrack->video)
	{
/* Failed */
		mpeg3_delete_vtrack(file, new_vtrack);
		new_vtrack = 0;
	}


	return new_vtrack;
}

int mpeg3_delete_vtrack(mpeg3_t *file, mpeg3_vtrack_t *vtrack)
{
	if(vtrack->video) mpeg3video_delete(vtrack->video);
	if(vtrack->demuxer) mpeg3_delete_demuxer(vtrack->demuxer);
	if(vtrack->private_offsets)
	{
		if(vtrack->frame_offsets) free(vtrack->frame_offsets);
		if(vtrack->keyframe_numbers) free(vtrack->keyframe_numbers);
	}
	mpeg3_delete_cache(vtrack->frame_cache);

	int i;
	for(i = 0; i < vtrack->total_subtitles; i++)
	{
		mpeg3_delete_subtitle(vtrack->subtitles[i]);
	}
	if(vtrack->subtitles) free(vtrack->subtitles);
	free(vtrack);
	return 0;
}

static int last_keyframe = 0;
void mpeg3_append_frame(mpeg3_vtrack_t *vtrack, int64_t offset, int is_keyframe)
{
	if(vtrack->total_frame_offsets >= vtrack->frame_offsets_allocated)
	{
		vtrack->frame_offsets_allocated = 
			MAX(vtrack->total_frame_offsets * 2, 1024);
		vtrack->frame_offsets = realloc(vtrack->frame_offsets,
			sizeof(int64_t) * vtrack->frame_offsets_allocated);
	}

	vtrack->frame_offsets[vtrack->total_frame_offsets++] = offset;

	if(is_keyframe)
	{
		if(vtrack->total_keyframe_numbers >= vtrack->keyframe_numbers_allocated)
		{
			vtrack->keyframe_numbers_allocated = 
				MAX(vtrack->total_keyframe_numbers * 2, 1024);
			vtrack->keyframe_numbers = realloc(vtrack->keyframe_numbers,
				sizeof(int64_t) * vtrack->keyframe_numbers_allocated);
		}

// Because the frame offsets are for the frame
// after, this needs to take off one frame.
		int corrected_frame = vtrack->total_frame_offsets - 2;
		if(corrected_frame < 0) corrected_frame = 0;
		vtrack->keyframe_numbers[vtrack->total_keyframe_numbers++] = 
			corrected_frame;
	}

	vtrack->private_offsets = 1;
}






