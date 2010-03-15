#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "mpeg3video.h"
#include <stdlib.h>
#include <string.h>

void mpeg3video_toc_error()
{
	fprintf(stderr, 
		"mpeg3video_seek: frame accurate seeking without a table of contents \n"
		"is no longer supported.  Use mpeg3toc <mpeg file> <table of contents>\n"
		"to generate a table of contents and load the table of contents instead.\n");
}

int mpeg3video_drop_frames(mpeg3video_t *video, int frames, int cache_it)
{
	int result = 0;
	int frame_number = video->framenum + frames;
	mpeg3_vtrack_t *track = video->track;

/* Read the selected number of frames and skip b-frames */
	while(!result && frame_number > video->framenum)
	{
		if(cache_it)
		{
			result = mpeg3video_read_frame_backend(video, 0);
			if(video->output_src[0])
			{
				mpeg3_cache_put_frame(track->frame_cache,
					video->framenum - 1,
					video->output_src[0],
					video->output_src[1],
					video->output_src[2],
					video->coded_picture_width * video->coded_picture_height,
					video->chrom_width * video->chrom_height,
					video->chrom_width * video->chrom_height);
			}
		}
		else
		{
			result = mpeg3video_read_frame_backend(video, frame_number - video->framenum);
		}
	}

	return result;
}

unsigned int mpeg3bits_next_startcode(mpeg3_bits_t* stream)
{
/* Perform forwards search */
	mpeg3bits_byte_align(stream);

	while(1)
	{
		unsigned int code = mpeg3bits_showbits32_noptr(stream);

		if((code >> 8) == MPEG3_PACKET_START_CODE_PREFIX) break;
		if(mpeg3bits_eof(stream)) break;

		mpeg3bits_getbyte_noptr(stream);
	}
	return mpeg3bits_showbits32_noptr(stream);
}

/* Line up on the beginning of the next code. */
int mpeg3video_next_code(mpeg3_bits_t* stream, unsigned int code)
{
	while(!mpeg3bits_eof(stream) && 
		mpeg3bits_showbits32_noptr(stream) != code)
	{
		mpeg3bits_getbyte_noptr(stream);
	}
	return mpeg3bits_eof(stream);
}

/* Line up on the beginning of the previous code. */
int mpeg3video_prev_code(mpeg3_demuxer_t *demuxer, unsigned int code)
{
	uint32_t current_code = 0;

#define PREV_CODE_MACRO \
{ \
	current_code >>= 8; \
	current_code |= ((uint32_t)mpeg3demux_read_prev_char(demuxer)) << 24; \
}

	PREV_CODE_MACRO
	PREV_CODE_MACRO
	PREV_CODE_MACRO
	PREV_CODE_MACRO

	while(!mpeg3demux_bof(demuxer) && current_code != code)
	{
		PREV_CODE_MACRO
	}
	return mpeg3demux_bof(demuxer);
}

long mpeg3video_goptimecode_to_frame(mpeg3video_t *video)
{
	return (long)(video->gop_timecode.hour * 3600 * video->frame_rate + 
		video->gop_timecode.minute * 60 * video->frame_rate +
		video->gop_timecode.second * video->frame_rate +
		video->gop_timecode.frame) - 1 - video->first_frame;
}

int mpeg3video_match_refframes(mpeg3video_t *video)
{
	unsigned char *dst, *src;
	int i, j, size;

	for(i = 0; i < 3; i++)
	{
		if(video->newframe[i])
		{
			if(video->newframe[i] == video->refframe[i])
			{
				src = video->refframe[i];
				dst = video->oldrefframe[i];
			}
			else
			{
				src = video->oldrefframe[i];
				dst = video->refframe[i];
			}

			if(i == 0)
				size = video->coded_picture_width * video->coded_picture_height + 32 * video->coded_picture_width;
			else 
				size = video->chrom_width * video->chrom_height + 32 * video->chrom_width;

			memcpy(dst, src, size);
		}
	}
	return 0;
}

int mpeg3video_seek_byte(mpeg3video_t *video, int64_t byte)
{
	mpeg3_t *file = video->file;
	mpeg3_bits_t *vstream = video->vstream;
	mpeg3_demuxer_t *demuxer = vstream->demuxer;

	video->byte_seek = byte;

	mpeg3bits_seek_byte(vstream, byte);
	return 0;
}

int mpeg3video_seek_frame(mpeg3video_t *video, long frame)
{
	video->frame_seek = frame;
	return 0;
}

int mpeg3_rewind_video(mpeg3video_t *video)
{
	mpeg3_bits_t *vstream = video->vstream;

	if(video->track->keyframes)
		mpeg3bits_seek_byte(vstream, video->track->keyframes[0].offset);
	else
		mpeg3bits_seek_byte(vstream, video->file->base_offset);
	video->last_number = -1;
	return 0;
}

int mpeg3video_seek(mpeg3video_t *video)
{
	long this_gop_start;
	int result = 0;
	int back_step;
	int attempts;
	mpeg3_t *file = video->file;
	mpeg3_bits_t *vstream = video->vstream;
	mpeg3_vtrack_t *track = video->track;
	mpeg3_demuxer_t *demuxer = vstream->demuxer;
	int64_t byte;
	int frame_number;


/* Must do seeking here so files which don't use video don't seek. */
/* Seeking is done in the demuxer */
/* Seek to absolute byte */
	if(video->byte_seek >= 0)
	{
		byte = video->byte_seek;
		video->byte_seek = -1;
		mpeg3demux_seek_byte(demuxer, byte);
// Rewind 2 I-frames
		if(byte > 0)
		{
			mpeg3demux_start_reverse(demuxer);

			if(!result)
			{
				if(video->has_gops)
					result = mpeg3video_prev_code(demuxer, MPEG3_GOP_START_CODE);
				else
					result = mpeg3video_prev_code(demuxer, MPEG3_SEQUENCE_START_CODE);
			}

			if(!result)
			{
				if(video->has_gops)
					result = mpeg3video_prev_code(demuxer, MPEG3_GOP_START_CODE);
				else
					result = mpeg3video_prev_code(demuxer, MPEG3_SEQUENCE_START_CODE);
			}

			mpeg3demux_start_forward(demuxer);
		}
		else
		{
// Read first frame
			video->repeat_count = 0;
			mpeg3bits_reset(vstream);
			mpeg3video_read_frame_backend(video, 0);
			mpeg3_rewind_video(video);
			video->repeat_count = 0;
		}

		mpeg3bits_reset(vstream);

// Read up to the correct byte
		result = 0;
		video->repeat_count = 0;
		while(!result && 
			!mpeg3demux_eof(demuxer) &&
			mpeg3demux_tell_byte(demuxer) < byte)
		{
			result = mpeg3video_read_frame_backend(video, 0);
		}

		mpeg3demux_reset_pts(demuxer);

	}
	else
/* Seek to a frame */
	if(video->frame_seek >= 0)
	{

		frame_number = video->frame_seek;
		video->frame_seek = -1;
		if(frame_number > video->maxframe) 
			frame_number = video->maxframe;
// Nothing to do: seek to next frame
		if(video->last_number > 0 && 
				frame_number == video->last_number + 1)
			return 0;

/* Seek to I frame in table of contents. */
		if(track->keyframes)
		{
			mpeg3_reset_cache(track->frame_cache);

			if((frame_number < video->framenum || 
				frame_number - video->framenum > MPEG3_SEEK_THRESHOLD))
			{
				int i;
				int gop_open = 1;
				for(i = track->total_keyframes - 1; i >= 0; i--)
				{
					if(track->keyframes[i].number <= frame_number)
					{
						int frame;
						int64_t byte;

						byte = track->keyframes[i].offset;

						frame = track->keyframes[i].number;
						video->framenum = frame - 1;
						video->last_number = -1;
						mpeg3bits_seek_byte(vstream, byte);
						mpeg3demux_reset_pts(demuxer);

						video->repeat_count = 0;

						mpeg3video_read_frame_backend(video, 0);
// On open-gop seek one more keyframe back
						if(gop_open && i && video->closed_gop == 0)
						{
							gop_open = 0;
							continue;
						}
// Forget previos refframes
						mpeg3video_match_refframes(video); 

// Read up to current frame
						mpeg3video_drop_frames(video, frame_number - video->framenum, 0);
						break;
					}
				}
			}
			else
			{
				video->repeat_count = 0;
				mpeg3video_drop_frames(video, frame_number - video->framenum, 0);
			}
		}
		else
/* No support for seeking without table of contents */
		{
			mpeg3video_toc_error();
		}



	}

	return result;
}

int mpeg3video_previous_frame(mpeg3video_t *video)
{
	mpeg3_bits_t *bitstream = video->vstream;
	mpeg3_demuxer_t *demuxer = bitstream->demuxer;
	int result = 0;
	int64_t target_byte = 0;

	if(mpeg3demux_tell_byte(demuxer) <= 0) return 1;

// Get location of end of previous picture
	mpeg3demux_start_reverse(demuxer);
	result = mpeg3video_prev_code(demuxer, MPEG3_PICTURE_START_CODE);
	if(!result) result = mpeg3video_prev_code(demuxer, MPEG3_PICTURE_START_CODE);
	if(!result) result = mpeg3video_prev_code(demuxer, MPEG3_PICTURE_START_CODE);
	if(!result) target_byte = mpeg3demux_tell_byte(demuxer);


// Rewind 2 I-frames
	if(!result)
	{
		if(video->has_gops)
			result = mpeg3video_prev_code(demuxer, MPEG3_GOP_START_CODE);
		else
			result = mpeg3video_prev_code(demuxer, MPEG3_SEQUENCE_START_CODE);
	}

	if(!result)
	{
		if(video->has_gops)
			result = mpeg3video_prev_code(demuxer, MPEG3_GOP_START_CODE);
		else
			result = mpeg3video_prev_code(demuxer, MPEG3_SEQUENCE_START_CODE);
	}

	mpeg3demux_start_forward(demuxer);
	mpeg3bits_reset(bitstream);

// Read up to correct byte
	result = 0;
	video->repeat_count = 0;
	while(!result && 
		!mpeg3demux_eof(demuxer) &&
		mpeg3demux_tell_byte(demuxer) < target_byte)
	{
		result = mpeg3video_read_frame_backend(video, 0);
	}

	video->repeat_count = 0;
	return 0;
}
