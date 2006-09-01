#include "../libmpeg3.h"
#include "../mpeg3private.h"
#include "../mpeg3protos.h"
#include "mpeg3video.h"
#include "mpeg3videoprotos.h"
#include <pthread.h>
#include <stdlib.h>

// "½Åµ¿ÈÆ" <doogle@shinbiro.com>


/* zig-zag scan */
unsigned char mpeg3_zig_zag_scan_nommx[64] =
{
  0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

/* alternate scan */
unsigned char mpeg3_alternate_scan_nommx[64] =
{
  0, 8, 16, 24, 1, 9, 2, 10, 17, 25, 32, 40, 48, 56, 57, 49, 
  41, 33, 26, 18, 3, 11, 4, 12, 19, 27, 34, 42, 50, 58, 35, 43, 
  51, 59, 20, 28, 5, 13, 6, 14, 21, 29, 36, 44, 52, 60, 37, 45, 
  53, 61, 22, 30, 7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

/* default intra quantization matrix */
unsigned char mpeg3_default_intra_quantizer_matrix[64] =
{
  8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
};

unsigned char mpeg3_non_linear_mquant_table[32] = 
{
   0, 1, 2, 3, 4, 5, 6, 7,
   8, 10, 12, 14, 16, 18, 20, 22, 
  24, 28, 32, 36, 40, 44, 48, 52, 
  56, 64, 72, 80, 88, 96, 104, 112
};

double mpeg3_frame_rate_table[16] =
{
  0.0,   /* Pad */
  (double)24000.0/1001.0,       /* Official frame rates */
  (double)24.0,
  (double)25.0,
  (double)30000.0/1001.0,
  (double)30.0,
  (double)50.0,
  (double)60000.0/1001.0,
  (double)60.0,

  1,                    /* Unofficial economy rates */
  5, 
  10,
  12,
  15,
  0,
  0,
};

int mpeg3video_initdecoder(mpeg3video_t *video)
{
	int blk_cnt_tab[3] = {6, 8, 12};
	int cc;
  	int i;
	long size[4], padding[2];         /* Size of Y, U, and V buffers */

	if(!video->mpeg2)
	{
/* force MPEG-1 parameters */
    	video->prog_seq = 1;
    	video->prog_frame = 1;
    	video->pict_struct = FRAME_PICTURE;
    	video->frame_pred_dct = 1;
    	video->chroma_format = CHROMA420;
    	video->matrix_coefficients = 5;
	}

/* Get dimensions rounded to nearest multiple of coded macroblocks */
	video->mb_width = (video->horizontal_size + 15) / 16;
	video->mb_height = (video->mpeg2 && !video->prog_seq) ? 
					(2 * ((video->vertical_size + 31) / 32)) : 
					((video->vertical_size + 15) / 16);
	video->coded_picture_width = 16 * video->mb_width;
	video->coded_picture_height = 16 * video->mb_height;
	video->chrom_width = (video->chroma_format == CHROMA444) ? 
					video->coded_picture_width : 
					(video->coded_picture_width >> 1);
	video->chrom_height = (video->chroma_format != CHROMA420) ? 
					video->coded_picture_height : 
                    (video->coded_picture_height >> 1);
	video->blk_cnt = blk_cnt_tab[video->chroma_format - 1];

/* Get sizes of YUV buffers */
	padding[0] = 16 * video->coded_picture_width;
	size[0] = video->coded_picture_width * video->coded_picture_height + padding[0] * 2;

	padding[1] = 16 * video->chrom_width;
	size[1] = video->chrom_width * video->chrom_height + 2 * padding[1];

	size[2] = (video->llw * video->llh);
	size[3] = (video->llw * video->llh) / 4;

/* Allocate contiguous fragments for YUV buffers for hardware YUV decoding */
	video->yuv_buffer[0] = (unsigned char*)calloc(1, (size[0] + padding[0]) + 2 * (size[1] + padding[1]));
	video->yuv_buffer[1] = (unsigned char*)calloc(1, (size[0] + padding[0]) + 2 * (size[1] + padding[1]));
	video->yuv_buffer[2] = (unsigned char*)calloc(1, (size[0] + padding[0]) + 2 * (size[1] + padding[1]));

    if(video->scalable_mode == SC_SPAT)
	{
		video->yuv_buffer[3] = (unsigned char*)calloc(1, size[2] + 2 * size[3]);
		video->yuv_buffer[4] = (unsigned char*)calloc(1, size[2] + 2 * size[3]);
	}

/* Direct pointers to areas of contiguous fragments in YVU order per Microsoft */	
	for(cc = 0; cc < 3; cc++)
	{
		video->llframe0[cc] = 0;
		video->llframe1[cc] = 0;
		video->newframe[cc] = 0;
	}

	video->refframe[0]    = video->yuv_buffer[0];
	video->oldrefframe[0] = video->yuv_buffer[1];
	video->auxframe[0]    = video->yuv_buffer[2];
	video->refframe[2]    = video->yuv_buffer[0] + size[0] + padding[0];
	video->oldrefframe[2] = video->yuv_buffer[1] + size[0] + padding[0];
	video->auxframe[2]    = video->yuv_buffer[2] + size[0] + padding[0];
	video->refframe[1]    = video->yuv_buffer[0] + size[0] + padding[0] + size[1] + padding[1];
	video->oldrefframe[1] = video->yuv_buffer[1] + size[0] + padding[0] + size[1] + padding[1];
	video->auxframe[1]    = video->yuv_buffer[2] + size[0] + padding[0] + size[1] + padding[1];

    if(video->scalable_mode == SC_SPAT)
	{
/* this assumes lower layer is 4:2:0 */
		video->llframe0[0] = video->yuv_buffer[3] + padding[0] 				   ;
		video->llframe1[0] = video->yuv_buffer[4] + padding[0] 				   ;
		video->llframe0[2] = video->yuv_buffer[3] + padding[1] + size[2]		   ;
		video->llframe1[2] = video->yuv_buffer[4] + padding[1] + size[2]		   ;
		video->llframe0[1] = video->yuv_buffer[3] + padding[1] + size[2] + size[3];
		video->llframe1[1] = video->yuv_buffer[4] + padding[1] + size[2] + size[3];
    }

/* Initialize the YUV tables for software YUV decoding */
	video->cr_to_r = malloc(sizeof(long) * 256);
	video->cr_to_g = malloc(sizeof(long) * 256);
	video->cb_to_g = malloc(sizeof(long) * 256);
	video->cb_to_b = malloc(sizeof(long) * 256);
	video->cr_to_r_ptr = video->cr_to_r + 128;
	video->cr_to_g_ptr = video->cr_to_g + 128;
	video->cb_to_g_ptr = video->cb_to_g + 128;
	video->cb_to_b_ptr = video->cb_to_b + 128;

	for(i = -128; i < 128; i++)
	{
		video->cr_to_r_ptr[i] = (long)( 1.371 * 65536 * i);
		video->cr_to_g_ptr[i] = (long)(-0.698 * 65536 * i);
		video->cb_to_g_ptr[i] = (long)(-0.336 * 65536 * i);
		video->cb_to_b_ptr[i] = (long)( 1.732 * 65536 * i);
	}

	return 0;
}

int mpeg3video_deletedecoder(mpeg3video_t *video)
{
	int i, padding;

	if(video->yuv_buffer[0]) free(video->yuv_buffer[0]);
	if(video->yuv_buffer[1]) free(video->yuv_buffer[1]);
	if(video->yuv_buffer[2]) free(video->yuv_buffer[2]);

	if(video->subtitle_frame[0]) free(video->subtitle_frame[0]);
	if(video->subtitle_frame[1]) free(video->subtitle_frame[1]);
	if(video->subtitle_frame[2]) free(video->subtitle_frame[2]);

	if(video->llframe0[0])
	{
		free(video->yuv_buffer[3]);
		free(video->yuv_buffer[4]);
	}

	free(video->cr_to_r);
	free(video->cr_to_g);
	free(video->cb_to_g);
	free(video->cb_to_b);
	return 0;
}

void mpeg3video_init_scantables(mpeg3video_t *video)
{
	{
		video->mpeg3_zigzag_scan_table = mpeg3_zig_zag_scan_nommx;
		video->mpeg3_alternate_scan_table = mpeg3_alternate_scan_nommx;
	}
}

mpeg3video_t* mpeg3video_allocate_struct(mpeg3_t *file, mpeg3_vtrack_t *track)
{
	int i;
	mpeg3video_t *video = calloc(1, sizeof(mpeg3video_t));
	pthread_mutexattr_t mutex_attr;

	video->file = file;
	video->track = track;
	video->vstream = mpeg3bits_new_stream(file, track->demuxer);
//printf("mpeg3video_allocate_struct %d\n", mpeg3bits_eof(video->vstream));
	video->last_number = -1;

/* First frame is all green */
	video->framenum = -1;

	video->byte_seek = -1;
	video->frame_seek = -1;

	mpeg3video_init_scantables(video);
	mpeg3video_init_output();

	pthread_mutexattr_init(&mutex_attr);
//	pthread_mutexattr_setkind_np(&mutex_attr, PTHREAD_MUTEX_FAST_NP);
	pthread_mutex_init(&(video->test_lock), &mutex_attr);
	pthread_mutex_init(&(video->slice_lock), &mutex_attr);
	return video;
}

int mpeg3video_delete_struct(mpeg3video_t *video)
{
	int i;
	mpeg3bits_delete_stream(video->vstream);
	pthread_mutex_destroy(&(video->test_lock));
	pthread_mutex_destroy(&(video->slice_lock));
	if(video->x_table)
	{
		free(video->x_table);
		free(video->y_table);
	}
	if(video->total_slice_decoders)
	{
		for(i = 0; i < video->total_slice_decoders; i++)
			mpeg3_delete_slice_decoder(&(video->slice_decoders[i]));
	}
	for(i = 0; i < video->slice_buffers_initialized; i++)
		mpeg3_delete_slice_buffer(&(video->slice_buffers[i]));


	free(video);
	return 0;
}


int mpeg3video_read_frame_backend(mpeg3video_t *video, int skip_bframes)
{
	int result = 0;
	int got_top = 0, got_bottom = 0;
	int i = 0;
	mpeg3_vtrack_t *track = video->track;
	mpeg3_t *file = (mpeg3_t*)video->file;

//printf("mpeg3video_read_frame_backend 1\n");

	do
	{
		if(mpeg3bits_eof(video->vstream)) result = 1;

		if(!result) result = mpeg3video_get_header(video, 0);


/* skip_bframes is the number of bframes we can skip successfully. */
/* This is in case a skipped B-frame is repeated and the second repeat happens */
/* to be a B frame we need. */
		video->skip_bframes = skip_bframes;

		if(!result)
			result = mpeg3video_getpicture(video, video->framenum);


		if(video->pict_struct == TOP_FIELD)
		{
			got_top == 1;
		}
		else
		if(video->pict_struct == BOTTOM_FIELD)
		{
			got_bottom = 1;
			video->secondfield = 0;
		}
		else
		if(video->pict_struct == FRAME_PICTURE)
		{
			got_top = got_bottom = 1;
		}

		i++;
	}while(i < 2 && 
		!got_bottom && 
		video->framenum >= 0);
// The way they do field based encoding, 
// the I frames have the top field but both the I frame and
// subsequent P frame are interlaced to make the keyframe.



//printf("mpeg3video_read_frame_backend 10\n");

// Composite subtitles
	mpeg3_decode_subtitle(video);




	if(!result)
	{
		video->last_number = video->framenum;
		video->framenum++;
	}
//printf("mpeg3video_read_frame_backend 100\n");

	return result;
}

int* mpeg3video_get_scaletable(int input_w, int output_w)
{
	int *result = malloc(sizeof(int) * output_w);
	float i;
	float scale = (float)input_w / output_w;
	for(i = 0; i < output_w; i++)
	{
		result[(int)i] = (int)(scale * i);
	}
	return result;
}

/* Get the first I frame. */
int mpeg3video_get_firstframe(mpeg3video_t *video)
{
	int result = 0;
	video->repeat_count = video->current_repeat = 0;
	result = mpeg3video_read_frame_backend(video, 0);
	return result;
}

static long gop_to_frame(mpeg3video_t *video, mpeg3_timecode_t *gop_timecode)
{
	int hour, minute, second, frame, fps;
	long result;

// Mirror of what mpeg2enc does
	fps = (int)(video->frame_rate + 0.5);


	hour = gop_timecode->hour;
	minute = gop_timecode->minute;
	second = gop_timecode->second;
	frame = gop_timecode->frame;
	
	result = (long)hour * 60 * 60 * fps + 
		minute * 60 * fps + 
		second * fps +
		frame;
	
	return result;
}


/* ======================================================================= */
/*                                    ENTRY POINTS */
/* ======================================================================= */



mpeg3video_t* mpeg3video_new(mpeg3_t *file, 
	mpeg3_vtrack_t *track)
{
	mpeg3video_t *video;
	mpeg3_bits_t *bitstream;
	mpeg3_demuxer_t *demuxer;
	int result = 0;

	video = mpeg3video_allocate_struct(file, track);
	bitstream = video->vstream;
	demuxer = bitstream->demuxer;

// Get encoding parameters from stream
	if(file->seekable)
	{
		result = mpeg3video_get_header(video, 1);

		if(!result)
		{
			int hour, minute, second, frame;
			int gop_found;

			mpeg3video_initdecoder(video);
			video->decoder_initted = 1;
			track->width = video->horizontal_size;
			track->height = video->vertical_size;
			track->frame_rate = video->frame_rate;

/* Try to get the length of the file from GOP's */
			if(!track->frame_offsets)
			{
				if(file->is_video_stream)
				{
/* Load the first GOP */
					mpeg3_rewind_video(video);
					result = mpeg3video_next_code(bitstream, 
						MPEG3_GOP_START_CODE);
					if(!result) mpeg3bits_getbits(bitstream, 32);
					if(!result) result = mpeg3video_getgophdr(video);

					hour = video->gop_timecode.hour;
					minute = video->gop_timecode.minute;
					second = video->gop_timecode.second;
					frame = video->gop_timecode.frame;

					video->first_frame = gop_to_frame(video, &video->gop_timecode);

/*
 * 			video->first_frame = (long)(hour * 3600 * video->frame_rate + 
 * 				minute * 60 * video->frame_rate +
 * 				second * video->frame_rate +
 * 				frame);
 */

/* GOPs are supposed to have 16 frames */

					video->frames_per_gop = 16;

/* Read the last GOP in the file by seeking backward. */
					mpeg3demux_seek_byte(demuxer, 
						mpeg3demux_movie_size(demuxer));
					mpeg3demux_start_reverse(demuxer);
					result = mpeg3video_prev_code(demuxer, 
						MPEG3_GOP_START_CODE);
					mpeg3demux_start_forward(demuxer);



					mpeg3bits_reset(bitstream);
					mpeg3bits_getbits(bitstream, 8);
					if(!result) result = mpeg3video_getgophdr(video);

					hour = video->gop_timecode.hour;
					minute = video->gop_timecode.minute;
					second = video->gop_timecode.second;
					frame = video->gop_timecode.frame;

					video->last_frame = gop_to_frame(video, &video->gop_timecode);

/*
 * 			video->last_frame = (long)((double)hour * 3600 * video->frame_rate + 
 * 				minute * 60 * video->frame_rate +
 * 				second * video->frame_rate +
 * 				frame);
 */

//printf("mpeg3video_new 3 %p\n", video);

/* Count number of frames to end */
					while(!result)
					{
						result = mpeg3video_next_code(bitstream, MPEG3_PICTURE_START_CODE);
						if(!result)
						{
							mpeg3bits_getbyte_noptr(bitstream);
							video->last_frame++;
						}
					}

					track->total_frames = video->last_frame - video->first_frame + 1;
//printf("mpeg3video_new 3 %ld\n", track->total_frames);
					mpeg3_rewind_video(video);
				}
				else
// Try to get the length of the file from the multiplexing.
// Need a table of contents
				{
/*
 * 				video->first_frame = 0;
 * 				track->total_frames = video->last_frame = 
 * 					(long)(mpeg3demux_length(video->vstream->demuxer) * 
 * 						video->frame_rate);
 * 				video->first_frame = 0;
 */
				}
			}
			else
// Get length from table of contents
			{
				track->total_frames = track->total_frame_offsets;
			}



			video->maxframe = track->total_frames;
			video->repeat_count = 0;
			mpeg3_rewind_video(video);
			mpeg3video_get_firstframe(video);
		}
		else
		{
			mpeg3video_delete(video);
			video = 0;
		}
	}

	return video;
}

int mpeg3video_delete(mpeg3video_t *video)
{
	if(video->decoder_initted)
	{
		mpeg3video_deletedecoder(video);
	}
	mpeg3video_delete_struct(video);
	return 0;
}

int mpeg3video_set_cpus(mpeg3video_t *video, int cpus)
{
	return 0;
}

int mpeg3video_set_mmx(mpeg3video_t *video, int use_mmx)
{
	mpeg3video_init_scantables(video);
	return 0;
}

/* Read all the way up to and including the next picture start code */
int mpeg3video_read_raw(mpeg3video_t *video, 
	unsigned char *output, 
	long *size, 
	long max_size)
{
	u_int32_t code = 0;
	mpeg3_bits_t *vstream = video->vstream;

	*size = 0;
	while(code != MPEG3_PICTURE_START_CODE && 
		code != MPEG3_SEQUENCE_END_CODE &&
		*size < max_size && 
		!mpeg3bits_eof(vstream))
	{
		code <<= 8;
		*output = mpeg3bits_getbyte_noptr(vstream);
		code |= *output++;
		(*size)++;
	}
	return mpeg3bits_eof(vstream);
}












int mpeg3video_read_frame(mpeg3video_t *video, 
		unsigned char **output_rows,
		int in_x, 
		int in_y, 
		int in_w, 
		int in_h, 
		int out_w, 
		int out_h, 
		int color_model)
{
	int result = 0;
	mpeg3_vtrack_t *track = video->track;

	video->want_yvu = 0;
	video->output_rows = output_rows;
	video->color_model = color_model;

/* Get scaling tables */
	if(video->out_w != out_w || video->out_h != out_h ||
		video->in_w != in_w || video->in_h != in_h ||
		video->in_x != in_x || video->in_y != in_y)
	{
		if(video->x_table)
		{
			free(video->x_table);
			free(video->y_table);
			video->x_table = 0;
			video->y_table = 0;
		}
	}

	video->out_w = out_w;
	video->out_h = out_h;
	video->in_w = in_w;
	video->in_h = in_h;
	video->in_x = in_x;
	video->in_y = in_y;

	if(!video->x_table)
	{
		video->x_table = mpeg3video_get_scaletable(video->in_w, video->out_w);
		video->y_table = mpeg3video_get_scaletable(video->in_h, video->out_h);
	}
//printf("mpeg3video_read_frame 1 %d\n", video->framenum);


// Recover from cache
	unsigned char *y, *u, *v;
	int frame_number = video->frame_seek >= 0 ? video->frame_seek : video->framenum;
	if(mpeg3_cache_get_frame(track->frame_cache, 
		frame_number, 
		&y, 
		&u, 
		&v))
	{
//printf("mpeg3video_read_frame 1 %d\n", frame_number);
// Swap output data for cache data
		unsigned char *temp[3];
		temp[0] = video->output_src[0];
		temp[1] = video->output_src[1];
		temp[2] = video->output_src[2];

		video->output_src[0] = y;
		video->output_src[1] = u;
		video->output_src[2] = v;
// Transfer with cropping
		if(video->output_src[0]) mpeg3video_present_frame(video);
		video->output_src[0] = temp[0];
		video->output_src[1] = temp[1];
		video->output_src[2] = temp[2];

// Advance either framenum or frame_seek
		if(frame_number == video->framenum)
			video->framenum = ++frame_number;
		else
		if(frame_number == video->frame_seek)
			video->frame_seek = ++frame_number;
	}
	else
	{

// Only decode if it's a different frame
		if(video->frame_seek < 0 || 
			video->last_number < 0 ||
			video->frame_seek != video->last_number)
		{
			if(!result) result = mpeg3video_seek(video);
			if(!result) result = mpeg3video_read_frame_backend(video, 0);
		}
		else
		{
			video->framenum = video->frame_seek + 1;
			video->last_number = video->frame_seek;
			video->frame_seek = -1;
		}

		if(video->output_src[0]) mpeg3video_present_frame(video);
	}

	return result;
}

int mpeg3video_read_yuvframe(mpeg3video_t *video, 
					char *y_output,
					char *u_output,
					char *v_output,
					int in_x,
					int in_y,
					int in_w,
					int in_h)
{
	int result = 0;
	mpeg3_vtrack_t *track = video->track;

//printf("mpeg3video_read_yuvframe 1 %d\n", video->framenum);
	video->want_yvu = 1;
	video->y_output = y_output;
	video->u_output = u_output;
	video->v_output = v_output;
	video->in_x = in_x;
	video->in_y = in_y;
	video->in_w = in_w;
	video->in_h = in_h;


// Recover from cache if framenum exists
	unsigned char *y, *u, *v;
	int frame_number = video->frame_seek >= 0 ? video->frame_seek : video->framenum;
	
	if(mpeg3_cache_get_frame(track->frame_cache, frame_number, &y, &u, &v))
	{
		int chroma_denominator;
		int size0, size1;


//printf("mpeg3video_read_yuvframe 1 %d\n", frame_number);
		if(video->chroma_format == CHROMA420)
			chroma_denominator = 2;
		else
			chroma_denominator = 1;
		size0 = video->coded_picture_width * video->in_h;
		size1 = video->chrom_width * (int)((float)video->in_h / chroma_denominator + 0.5);


// Swap output data for cache data
		unsigned char *temp[3];
		temp[0] = video->output_src[0];
		temp[1] = video->output_src[1];
		temp[2] = video->output_src[2];

		video->output_src[0] = y;
		video->output_src[1] = u;
		video->output_src[2] = v;
// Transfer with cropping
		if(video->output_src[0]) mpeg3video_present_frame(video);
		video->output_src[0] = temp[0];
		video->output_src[1] = temp[1];
		video->output_src[2] = temp[2];

// Advance either framenum or frame_seek
		if(frame_number == video->framenum)
			video->framenum = ++frame_number;
		else
		if(frame_number == video->frame_seek)
			video->frame_seek = ++frame_number;
	}
	else
	{
		if(!result) result = mpeg3video_seek(video);
		if(!result) result = mpeg3video_read_frame_backend(video, 0);
		if(video->output_src[0]) mpeg3video_present_frame(video);
	}




	video->want_yvu = 0;
	video->byte_seek = -1;
	return result;
}

int mpeg3video_read_yuvframe_ptr(mpeg3video_t *video, 
					char **y_output,
					char **u_output,
					char **v_output)
{
	int result = 0;
	mpeg3_vtrack_t *track = video->track;

	video->want_yvu = 1;

	*y_output = *u_output = *v_output = 0;

	unsigned char *y, *u, *v;
	int frame_number = video->frame_seek >= 0 ? video->frame_seek : video->framenum;
	if(mpeg3_cache_get_frame(track->frame_cache, frame_number, &y, &u, &v))
	{
		*y_output = (char*)y;
		*u_output = (char*)u;
		*v_output = (char*)v;

// Advance either framenum or frame_seek
		if(frame_number == video->framenum)
			video->framenum = ++frame_number;
		else
		if(frame_number == video->frame_seek)
			video->frame_seek = ++frame_number;
	}
	else
// Only decode if it's a different frame
	if(video->frame_seek < 0 || 
		video->last_number < 0 ||
		video->frame_seek != video->last_number)
	{
		if(!result) result = mpeg3video_seek(video);
		if(!result) result = mpeg3video_read_frame_backend(video, 0);

        if(video->output_src[0])
        {
            *y_output = (char*)video->output_src[0];
            *u_output = (char*)video->output_src[1];
            *v_output = (char*)video->output_src[2];
        }
	}
	else
	{
		video->framenum = video->frame_seek + 1;
		video->last_number = video->frame_seek;
		video->frame_seek = -1;

        if(video->output_src[0])
        {
            *y_output = (char*)video->output_src[0];
            *u_output = (char*)video->output_src[1];
            *v_output = (char*)video->output_src[2];
        }
	}


	video->want_yvu = 0;
// Caching not used if byte seek
	video->byte_seek = -1;


	return result;
}

int mpeg3video_colormodel(mpeg3video_t *video)
{
	switch(video->chroma_format)
	{
		case CHROMA422:
			return MPEG3_YUV422P;
			break;

		case CHROMA420:
			return MPEG3_YUV420P;
			break;
	}

	return MPEG3_YUV420P;
}

void mpeg3video_dump(mpeg3video_t *video)
{
	printf("mpeg3video_dump 1\n");
	printf(" *** sequence extension 1\n");
	printf("prog_seq=%d\n", video->prog_seq);
	printf(" *** picture header 1\n");
	printf("pict_type=%d field_sequence=%d\n", video->pict_type, video->field_sequence);
	printf(" *** picture coding extension 1\n");
	printf("field_sequence=%d repeatfirst=%d prog_frame=%d pict_struct=%d\n", 
		video->field_sequence,
		video->repeatfirst, 
		video->prog_frame,
		video->pict_struct);
}






