#include "../mpeg3private.h"
#include "../mpeg3protos.h"
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

int mpeg3video_drop_frames(mpeg3video_t *video, long frames, int cache_it)
{
	int result = 0;
	long frame_number = video->framenum + frames;
	mpeg3_vtrack_t *track = video->track;
/* first 3 frames are garbage as proven by printfs in the read_frame calls */
	int drop_count = 3;

/* Read the selected number of frames and skip b-frames */
	while(!result && frame_number > video->framenum)
	{
		if(cache_it)
		{
			result = mpeg3video_read_frame_backend(video, 0);
        	if(video->output_src[0] && drop_count--)
        	{
				mpeg3_cache_put_frame(track->frame_cache,
					video->framenum - 1,
					video->output_src[0],
					video->output_src[1],
					video->output_src[2],
					video->coded_picture_width * video->coded_picture_height,
					video->chrom_width * video->chrom_height,
					video->chrom_width * video->chrom_height);
//printf("mpeg3video_drop_frames 1 %d\n", video->framenum);
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


/*
 * printf("mpeg3bits_next_startcode 1 %lld %lld\n", 
 * stream->demuxer->titles[0]->fs->current_byte, 
 * stream->demuxer->titles[0]->fs->total_bytes);
 * 
 */

//mpeg3_read_next_packet(stream->demuxer);
//printf("mpeg3bits_next_startcode 2 %d %d\n", 
//	stream->demuxer->titles[0]->fs->current_byte, 
//	stream->demuxer->titles[0]->fs->total_bytes);

//printf("mpeg3bits_next_startcode 2 %llx\n", mpeg3bits_tell(stream));
/* Perform search */
	while(1)
	{
		unsigned int code = mpeg3bits_showbits32_noptr(stream);

		if((code >> 8) == MPEG3_PACKET_START_CODE_PREFIX) break;
		if(mpeg3bits_eof(stream)) break;


		mpeg3bits_getbyte_noptr(stream);

/*
 * printf("mpeg3bits_next_startcode 3 %08x %d %d\n", 
 * mpeg3bits_showbits32_noptr(stream), 
 * stream->demuxer->titles[0]->fs->current_byte, 
 * stream->demuxer->titles[0]->fs->total_bytes);
 */

	}
//printf("mpeg3bits_next_startcode 4 %d %d\n", 
//	stream->demuxer->titles[0]->fs->current_byte, 
//	stream->demuxer->titles[0]->fs->total_bytes);
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
/*  printf("mpeg3video_goptimecode_to_frame %d %d %d %d %f\n",  */
/*  	video->gop_timecode.hour, video->gop_timecode.minute, video->gop_timecode.second, video->gop_timecode.frame, video->frame_rate); */
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



// Need PTS now so audio can be synchronized
	mpeg3bits_seek_byte(vstream, byte);
//	file->percentage_pts = mpeg3demux_scan_pts(demuxer);
	return 0;
}

int mpeg3video_seek_frame(mpeg3video_t *video, long frame)
{
	video->frame_seek = frame;
	return 0;
}

int mpeg3_rewind_video(mpeg3video_t *video)
{
	mpeg3_vtrack_t *track = video->track;
	mpeg3_bits_t *vstream = video->vstream;

	if(track->frame_offsets)
		mpeg3bits_seek_byte(vstream, track->frame_offsets[0]);
	else
		mpeg3bits_seek_byte(vstream, 0);

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
	long frame_number;
	int match_refframes = 1;



/* Must do seeking here so files which don't use video don't seek. */
/* Seeking is done in the demuxer */
/* Seek to absolute byte */
	if(video->byte_seek >= 0)
	{
		byte = video->byte_seek;
		video->byte_seek = -1;
		mpeg3demux_seek_byte(demuxer, byte);


// Clear subtitles
		mpeg3_reset_subtitles(file);


// Rewind 2 I-frames
		if(byte > 0)
		{
//printf("mpeg3video_seek 1\n");
			mpeg3demux_start_reverse(demuxer);

//printf("mpeg3video_seek 1 %lld\n", mpeg3demux_tell_byte(demuxer));
			if(!result)
			{
				if(video->has_gops)
					result = mpeg3video_prev_code(demuxer, MPEG3_GOP_START_CODE);
				else
					result = mpeg3video_prev_code(demuxer, MPEG3_SEQUENCE_START_CODE);
			}
//printf("mpeg3video_seek 2 %lld\n", mpeg3demux_tell_byte(demuxer));

			if(!result)
			{
				if(video->has_gops)
					result = mpeg3video_prev_code(demuxer, MPEG3_GOP_START_CODE);
				else
					result = mpeg3video_prev_code(demuxer, MPEG3_SEQUENCE_START_CODE);
			}

//printf("mpeg3video_seek 3 %lld\n", mpeg3demux_tell_byte(demuxer));



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

//printf("mpeg3video_seek 4 %lld\n", mpeg3demux_tell_byte(demuxer));
// Read up to the correct byte
		result = 0;
		video->repeat_count = 0;
		while(!result && 
			!mpeg3demux_eof(demuxer) &&
			mpeg3demux_tell_byte(demuxer) < byte)
		{
			result = mpeg3video_read_frame_backend(video, 0);
		}

//printf("mpeg3video_seek 5 %lld\n", mpeg3demux_tell_byte(demuxer));




		mpeg3demux_reset_pts(demuxer);

//printf("mpeg3video_seek 10\n");

	}
	else
/* Seek to a frame */
	if(video->frame_seek >= 0)
	{
// Clear subtitles
		mpeg3_reset_subtitles(file);


		frame_number = video->frame_seek;
		video->frame_seek = -1;
		if(frame_number < 0) frame_number = 0;
		if(frame_number > video->maxframe) frame_number = video->maxframe;

//printf("mpeg3video_seek 1 %ld %ld\n", frame_number, video->framenum);

/* Seek to I frame in table of contents. */
/* Determine time between seek position and previous subtitle. */
/* Subtract time difference from subtitle display time. */
		if(track->frame_offsets)
		{
			mpeg3_reset_cache(track->frame_cache);

			if((frame_number < video->framenum || 
				frame_number - video->framenum > MPEG3_SEEK_THRESHOLD))
			{
				int i;
				for(i = track->total_keyframe_numbers - 1; i >= 0; i--)
				{
					if(track->keyframe_numbers[i] <= frame_number)
					{
						int frame;
						int64_t byte;

// Go 2 I-frames before current position
						if(i > 0) i--;

						frame = track->keyframe_numbers[i];
						if(frame == 0)
							byte = track->frame_offsets[0];
						else
							byte = track->frame_offsets[frame];
						video->framenum = track->keyframe_numbers[i];

						mpeg3bits_seek_byte(vstream, byte);


// Get first 2 I-frames
						if(byte == 0)
						{
							mpeg3video_get_firstframe(video);
							mpeg3video_read_frame_backend(video, 0);
						}

					

						video->repeat_count = 0;

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



		mpeg3demux_reset_pts(demuxer);
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
