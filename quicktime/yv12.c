#include "colormodels.h"
#include "funcprotos.h"
#include "workarounds.h"
#include "yv12.h"

#ifndef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

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


static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	longest bytes, x, y;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	unsigned char *buffer;
	unsigned char *output_row0, *output_row1, *y_plane0, *y_plane1, *u_plane, *v_plane;
	longest y_size, u_size, v_size;
	int result = 0;
	int y1, u, v, y2, r, g, b, y3, y4;
	int bytes_per_row = width * cmodel_calculate_pixelsize(file->color_model);

	y_size = codec->coded_h * codec->coded_w;
	u_size = codec->coded_h * codec->coded_w / 4;
	v_size = codec->coded_h * codec->coded_w / 4;

	vtrack->track->tkhd.track_width;
	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);


//printf("yv12 decode %lld %lld %lld\n", y_size, u_size, v_size);
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
//printf("yv12 decode %d\n", file->color_model);
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
	longest offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int result = 0;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	unsigned char *y_plane0, *y_plane1, *u_plane, *v_plane;
	longest y_size, u_size, v_size;
	unsigned char *input_row0, *input_row1;
	int x, y;
	int y1, u, y2, v, y3, y4, subscript;
	int r, g, b;
	longest bytes = (longest)0;

	y_size = codec->coded_h * codec->coded_w;
	u_size = codec->coded_h * codec->coded_w / 4;
	v_size = codec->coded_h * codec->coded_w / 4;
	bytes = quicktime_add3(y_size, u_size, v_size);

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
//printf("quicktime_encode_yv12 1 %lld\n", bytes);

	quicktime_update_tables(file,
						file->vtracks[track].track,
						offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						bytes);
//printf("quicktime_encode_yv12 2\n");

	file->vtracks[track].current_chunk++;
	return result;
}

void quicktime_init_codec_yv12(quicktime_video_map_t *vtrack)
{
	int i;
	quicktime_yv12_codec_t *codec;

//printf("quicktime_init_codec_yv12 1\n");
/* Init public items */
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_yv12_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;

/* Init private items */
	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	codec->coded_w = (int)(vtrack->track->tkhd.track_width / 2);
	codec->coded_w *= 2;
	codec->coded_h = (int)(vtrack->track->tkhd.track_height / 2);
	codec->coded_h *= 2;
	cmodel_init_yuv(&codec->yuv_table);
	codec->work_buffer = malloc(codec->coded_w * codec->coded_h + 
		codec->coded_w * codec->coded_h / 2);
}

