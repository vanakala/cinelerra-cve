#include "quicktime.h"
#include "jpeglib.h"

#define ABS(x) ((x) < 0 ? -(x) : (x)) 

int quicktime_init_codec_wmx1(quicktime_video_map_t *vtrack)
{
	int i;
	quicktime_wmx1_codec_t *codec = &(vtrack->codecs.wmx1_codec);

	quicktime_init_yuv(&(codec->yuv_tables));
	codec->bytes_per_line = vtrack->track->tkhd.track_width * 3;
	if((float)codec->bytes_per_line / 6 > (int)(codec->bytes_per_line / 6))
		codec->bytes_per_line += 3;

	codec->rows = vtrack->track->tkhd.track_height / 2;
	if((float)vtrack->track->tkhd.track_height / 2 > (int)(vtrack->track->tkhd.track_height / 2))
		codec->rows++;

	for(i = 0; i < WMX_CHUNK_FRAMES; i++)
		codec->frame_cache[i] = 0;

	codec->key_frame = 0;
	codec->keyframe_position = 0;
	codec->quality = 100;
	codec->use_float = 0;
	codec->frames_per_chunk = 0;
	codec->threshold = 5;
	return 0;
}

int quicktime_delete_codec_wmx1(quicktime_video_map_t *vtrack)
{
	int i;
	quicktime_wmx1_codec_t *codec = &(vtrack->codecs.wmx1_codec);

	quicktime_delete_yuv(&(codec->yuv_tables));
	for(i = 0; i < WMX_CHUNK_FRAMES; i++)
		if(codec->frame_cache[i]) free(codec->frame_cache[i]);

	if(codec->key_frame) free(codec->key_frame);
	return 0;
}

int wmx1_write_cache(quicktime_t *file, int track)
{
	long offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_wmx1_codec_t *codec = &(vtrack->codecs.wmx1_codec);
	int width = codec->bytes_per_line;
	int height = codec->rows;
	int result = 0;
	int frame, channel, i, j, k;
	long bytes_per_row = codec->bytes_per_line;
	int step = codec->frames_per_chunk * bytes_per_row;
	long bytes = height * bytes_per_row * codec->frames_per_chunk;
	long output_bytes;
	unsigned char *input_row, *input_end, *output_row, *input_frame, *output_frame;

/*printf("wmx1_write_cache 1 %d %d %d\n", bytes_per_row, bytes_per_channel, bytes); */
	if(!codec->key_frame)
	{
		codec->key_frame = malloc(bytes);
		if(!codec->key_frame) result = 1;
	}
/*printf("wmx1_write_cache 2\n"); */

/* Interlace rows */
	for(frame = 0; frame < codec->frames_per_chunk; frame++)
	{
		input_frame = codec->frame_cache[frame];
		output_frame = codec->key_frame + frame * bytes_per_row;
		input_row = input_frame;
		output_row = output_frame;

		for(i = 0; i < height; i++)
		{
			for(j = 0; j < width; j++)
			{
				output_row[j] = input_row[j];
			}
			output_row += step;
			input_row += bytes_per_row;
		}
	}

/* Write as a jpeg */
/* 	{ */
/* 		struct jpeg_compress_struct jpeg_compress; */
/* 		struct jpeg_error_mgr jpeg_error; */
/* 		JSAMPROW row_pointer[1]; */
/* 		jpeg_compress.err = jpeg_std_error(&jpeg_error); */
/* 		jpeg_create_compress(&jpeg_compress); */
/* 		jpeg_stdio_dest(&jpeg_compress, quicktime_get_fd(file)); */
/* 		jpeg_compress.image_width = bytes_per_row; */
/* 		jpeg_compress.image_height = height * codec->frames_per_chunk; */
/* 		jpeg_compress.input_components = 1; */
/* 		jpeg_compress.in_color_space = JCS_GRAYSCALE; */
/* 		jpeg_set_defaults(&jpeg_compress); */
/* 		jpeg_set_quality(&jpeg_compress, codec->quality, 0); */
/* 		jpeg_start_compress(&jpeg_compress, TRUE); */
/* 		while(jpeg_compress.next_scanline < jpeg_compress.image_height && !result) */
/* 		{ */
/* 			row_pointer[0] = codec->key_frame + jpeg_compress.next_scanline * bytes_per_row; */
/* 			result = jpeg_write_scanlines(&jpeg_compress, row_pointer, 1); */
/* 			result = !result; */
/* 		} */
/* 		jpeg_finish_compress(&jpeg_compress); */
/* 		jpeg_destroy((j_common_ptr)&jpeg_compress); */
/* 	} */

/* Write as raw */
	result = quicktime_write_data(file, codec->key_frame, bytes);
	result = !result;
/*printf("wmx1_write_cache 4\n"); */

	output_bytes = quicktime_position(file) - offset;
	quicktime_update_tables(file,
						vtrack->track,
						offset,
						vtrack->current_chunk,
						vtrack->current_position,
						codec->frames_per_chunk,
						output_bytes);
/*printf("wmx1_write_cache 5\n"); */

	codec->frames_per_chunk = 0;
	vtrack->current_chunk++;
	return result;
}

/* Perform YUV transform and store in cache */
int wmx1_store_in_cache(quicktime_t *file, unsigned char *cache, unsigned char **row_pointers, int track)
{
	long offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_wmx1_codec_t *codec = &(vtrack->codecs.wmx1_codec);
	quicktime_yuv_t *yuv_tables = &(codec->yuv_tables);
	int result = 0;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	long bytes = codec->rows * codec->bytes_per_line;
	unsigned char *buffer = cache;
	unsigned char *output_row;    /* Pointer to output row */
	unsigned char *row_pointer1, *row_pointer2;  /* Pointers to input rows */
	int x1, x2, in_y, out_y;
	int y1, y2, y3, y4, u, v;
	int r, g, b;
	int endpoint = width * 3;
	int denominator;

	for(in_y = 0, out_y = 0; in_y < height; out_y++)
	{
		output_row = buffer + out_y * codec->bytes_per_line;
		row_pointer1 = row_pointers[in_y];
		in_y++;

		if(in_y < height)
			row_pointer2 = row_pointers[in_y];
		else
			row_pointer2 = row_pointer1;

		in_y++;

		for(x1 = 0, x2 = 0; x1 < endpoint; )
		{
/* Top left pixel */
			r = row_pointer1[x1++];
			g = row_pointer1[x1++];
			b = row_pointer1[x1++];

			y1 = (yuv_tables->rtoy_tab[r] + yuv_tables->gtoy_tab[g] + yuv_tables->btoy_tab[b]);
			u = (yuv_tables->rtou_tab[r] + yuv_tables->gtou_tab[g] + yuv_tables->btou_tab[b]);
			v = (yuv_tables->rtov_tab[r] + yuv_tables->gtov_tab[g] + yuv_tables->btov_tab[b]);

/* Top right pixel */
			if(x1 < endpoint)
			{
				r = row_pointer1[x1++];
				g = row_pointer1[x1++];
				b = row_pointer1[x1++];
			}

			y2 = (yuv_tables->rtoy_tab[r] + yuv_tables->gtoy_tab[g] + yuv_tables->btoy_tab[b]);
			u += (yuv_tables->rtou_tab[r] + yuv_tables->gtou_tab[g] + yuv_tables->btou_tab[b]);
			v += (yuv_tables->rtov_tab[r] + yuv_tables->gtov_tab[g] + yuv_tables->btov_tab[b]);

/* Bottom left pixel */
			r = row_pointer2[x2++];
			g = row_pointer2[x2++];
			b = row_pointer2[x2++];

			y3 = (yuv_tables->rtoy_tab[r] + yuv_tables->gtoy_tab[g] + yuv_tables->btoy_tab[b]);
			u += (yuv_tables->rtou_tab[r] + yuv_tables->gtou_tab[g] + yuv_tables->btou_tab[b]);
			v += (yuv_tables->rtov_tab[r] + yuv_tables->gtov_tab[g] + yuv_tables->btov_tab[b]);

/* Bottom right pixel */
			if(x2 < endpoint)
			{
				r = row_pointer2[x2++];
				g = row_pointer2[x2++];
				b = row_pointer2[x2++];
			}

			y4 = (yuv_tables->rtoy_tab[r] + yuv_tables->gtoy_tab[g] + yuv_tables->btoy_tab[b]);
			u += (yuv_tables->rtou_tab[r] + yuv_tables->gtou_tab[g] + yuv_tables->btou_tab[b]);
			v += (yuv_tables->rtov_tab[r] + yuv_tables->gtov_tab[g] + yuv_tables->btov_tab[b]);

			y1 /= 0x10000;
			y2 /= 0x10000;
			y3 /= 0x10000;
			y4 /= 0x10000;
			u /= 0x40000;
			v /= 0x40000;
			if(y1 > 255) y1 = 255;
			if(y2 > 255) y2 = 255;
			if(y3 > 255) y3 = 255;
			if(y4 > 255) y4 = 255;
			if(u > 127) u = 127;
			if(v > 127) v = 127;
			if(y1 < 0) y1 = 0;
			if(y2 < 0) y2 = 0;
			if(y3 < 0) y3 = 0;
			if(y4 < 0) y4 = 0;
			if(u < -128) u = -128;
			if(v < -128) v = -128;

			*output_row++ = u;
			*output_row++ = v;
			*output_row++ = y1;
			*output_row++ = y2;
			*output_row++ = y3;
			*output_row++ = y4;
		}
	}
	return 0;
}

int quicktime_encode_wmx1(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	int written_cache = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_wmx1_codec_t *codec = &(vtrack->codecs.wmx1_codec);
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int bytes = codec->rows * codec->bytes_per_line;
	long frame_difference = 0;
	int i;

/* Arm cache */
	if(codec->frames_per_chunk < WMX_CHUNK_FRAMES)
	{
		unsigned char *frame_cache;
		unsigned char *row_pointer1, *row_pointer2, *endpoint;
		if(!codec->frame_cache[codec->frames_per_chunk])
			codec->frame_cache[codec->frames_per_chunk] = malloc(bytes);
		frame_cache = codec->frame_cache[codec->frames_per_chunk];

/* Copy to cache */
		wmx1_store_in_cache(file, frame_cache, row_pointers, track);
		codec->frames_per_chunk++;
	}
	else
	{
/* Write cache and start new cache. */
		unsigned char *frame_cache;

		result = wmx1_write_cache(file, track);
		written_cache = 1;

/* Copy next frame to cache */
		if(!codec->frame_cache[codec->frames_per_chunk])
			codec->frame_cache[codec->frames_per_chunk] = malloc(bytes);
		frame_cache = codec->frame_cache[codec->frames_per_chunk];
		wmx1_store_in_cache(file, codec->frame_cache[codec->frames_per_chunk], row_pointers, track);
		codec->frames_per_chunk++;
	}
/*printf("quicktime_encode_wmx1 3\n"); */

/* 	if(!written_cache) */
/* 		quicktime_update_tables(file, */
/*						vtrack->track, */
/* 						offset, */
/* 						vtrack->current_chunk, */
/* 						vtrack->current_position, */
/* 						codec->frames_per_chunk, */
/* 						output_bytes); */
/*  */
	return result;
}

int quicktime_decode_wmx1(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	return result;
}
