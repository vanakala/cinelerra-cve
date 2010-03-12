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
	if(file->keyframes)
	{
		new_vtrack->demuxer->stream_end = file->video_eof[number];
		new_vtrack->keyframes = file->keyframes[number];
		new_vtrack->total_keyframes = file->total_keyframes[number];
		new_vtrack->total_frames = file->total_frames[number];
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
		if(vtrack->keyframes) free(vtrack->keyframes);
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
	vtrack->total_frames++;

	if(is_keyframe)
	{
		if(vtrack->total_keyframes >= vtrack->keyframes_allocated)
		{
			vtrack->keyframes_allocated = 
			    MAX(vtrack->total_keyframes * 2, 1024);
			vtrack->keyframes = realloc(vtrack->keyframes,
			    sizeof(mpeg3tocitem_t) * vtrack->keyframes_allocated);
		}
		vtrack->keyframes[vtrack->total_keyframes].offset = offset;
		vtrack->keyframes[vtrack->total_keyframes++].number = 
		    vtrack->total_frames - 1;
	}

	vtrack->private_offsets = 1;
}
