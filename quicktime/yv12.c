#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "workarounds.h"
#include "yv12.h"

#include <stdlib.h>

#ifndef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

typedef struct
{
	cmodel_yuv_t yuv_table;
	int coded_w, coded_h;
	unsigned char *work_buffer;
	int initialized;
} quicktime_yv12_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_yv12_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	free(codec->work_buffer);
	free(codec);
	return 0;
}

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUV420P);
}

static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_YUV420P);
}

static void initialize(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_yv12_codec_t *codec = codec_base->priv;
	if(!codec->initialized)
	{
/* Init private items */
		codec->coded_w = (int)(vtrack->track->tkhd.track_width / 2);
		codec->coded_w *= 2;
		codec->coded_h = (int)(vtrack->track->tkhd.track_height / 2);
		codec->coded_h *= 2;
		cmodel_init_yuv(&codec->yuv_table);
		codec->work_buffer = malloc(codec->coded_w * codec->coded_h + 
			codec->coded_w * codec->coded_h / 2);
		codec->initialized = 1;
	}
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int64_t bytes, x, y;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	unsigned char *buffer;
	unsigned char *output_row0, *output_row1, *y_plane0, *y_plane1, *u_plane, *v_plane;
	int64_t y_size, u_size, v_size;
	int result = 0;
	int y1, u, v, y2, r, g, b, y3, y4;
	int i;
	int bytes_per_row = width * cmodel_calculate_pixelsize(file->color_model);
	initialize(vtrack);

	y_size = codec->coded_h * codec->coded_w;
	u_size = codec->coded_h * codec->coded_w / 4;
	v_size = codec->coded_h * codec->coded_w / 4;

	vtrack->track->tkhd.track_width;
	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);


	if(file->color_model == BC_YUV420P && 
			file->in_x == 0 && 
			file->in_y == 0 && 
			file->in_w == width &&
			file->in_h == height &&
			file->out_w == width &&
			file->out_h == height)
	{
		result = !quicktime_read_data(file, row_pointers[0], y_size);
		result = !quicktime_read_data(file, row_pointers[1], u_size);
		result = !quicktime_read_data(file, row_pointers[2], v_size);
	}
	else
	{
		result = !quicktime_read_data(file, codec->work_buffer, bytes);
		cmodel_transfer(row_pointers, 
			0,
			row_pointers[0],
			row_pointers[1],
			row_pointers[2],
			codec->work_buffer,
			codec->work_buffer + y_size,
			codec->work_buffer + y_size + u_size,
			file->in_x, 
			file->in_y, 
			file->in_w, 
			file->in_h,
			0, 
			0, 
			file->out_w, 
			file->out_h,
			BC_YUV420P, 
			file->color_model,
			0,
			codec->coded_w,
			file->out_w);
	}

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int64_t offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int result = 0;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	unsigned char *y_plane0, *y_plane1, *u_plane, *v_plane;
	int64_t y_size, u_size, v_size;
	unsigned char *input_row0, *input_row1;
	int x, y;
	int y1, u, y2, v, y3, y4, subscript;
	int r, g, b;
	int64_t bytes = (int64_t)0;
	quicktime_atom_t chunk_atom;
	initialize(vtrack);

	y_size = codec->coded_h * codec->coded_w;
	u_size = codec->coded_h * codec->coded_w / 4;
	v_size = codec->coded_h * codec->coded_w / 4;
	bytes = quicktime_add3(y_size, u_size, v_size);

	quicktime_write_chunk_header(file, trak, &chunk_atom);
	if(file->color_model == BC_YUV420P)
	{
		result = !quicktime_write_data(file, row_pointers[0], y_size);
		if(!result) result = !quicktime_write_data(file, row_pointers[1], u_size);
		if(!result) result = !quicktime_write_data(file, row_pointers[2], v_size);
	}
	else
	{
		cmodel_transfer(0, 
			row_pointers,
			codec->work_buffer,
			codec->work_buffer + y_size,
			codec->work_buffer + y_size + u_size,
			row_pointers[0],
			row_pointers[1],
			row_pointers[2],
			0, 
			0, 
			width, 
			height,
			0, 
			0, 
			width, 
			height,
			file->color_model,
			BC_YUV420P, 
			0,
			width,
			codec->coded_w);
		result = !quicktime_write_data(file, codec->work_buffer, bytes);
	}

	quicktime_write_chunk_footer(file, 
		trak,
		vtrack->current_chunk,
		&chunk_atom, 
		1);

	vtrack->current_chunk++;
	return result;
}

void quicktime_init_codec_yv12(quicktime_video_map_t *vtrack)
{
	int i;
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_yv12_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->fourcc = QUICKTIME_YUV420;
	codec_base->title = "YUV 4:2:0 Planar";
	codec_base->desc = "YUV 4:2:0 Planar";
}

