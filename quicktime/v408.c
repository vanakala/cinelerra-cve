#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "workarounds.h"
#include "v408.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct
{
	unsigned char *work_buffer;
} quicktime_v408_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_v408_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
	free(codec);
	return 0;
}

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_RGBA8888 ||
		colormodel == BC_RGB161616 ||
		colormodel == BC_RGBA16161616 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUVA8888 ||
		colormodel == BC_YUV161616 ||
		colormodel == BC_YUVA16161616 ||
		colormodel == BC_RGB8 ||
		colormodel == BC_RGB565 ||
		colormodel == BC_BGR888 ||
		colormodel == BC_BGR8888);
}

static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_RGBA8888 ||
		colormodel == BC_RGB161616 ||
		colormodel == BC_RGBA16161616 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUVA8888 ||
		colormodel == BC_YUV161616 ||
		colormodel == BC_YUVA16161616);
}














static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int i;
	int64_t bytes;
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v408_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	unsigned char **input_rows;
	if(!codec->work_buffer)
		codec->work_buffer = malloc(vtrack->track->tkhd.track_width * 
			vtrack->track->tkhd.track_height *
			4);


	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	result = !quicktime_read_data(file, codec->work_buffer, bytes);



	input_rows = malloc(sizeof(unsigned char*) * height);
	for(i = 0; i < height; i++)
		input_rows[i] = codec->work_buffer + i * width * 4;

	cmodel_transfer(row_pointers, 
		input_rows,
		row_pointers[0],
		row_pointers[1],
		row_pointers[2],
		0,
		0,
		0,
		file->in_x, 
		file->in_y, 
		file->in_w, 
		file->in_h,
		0, 
		0, 
		file->out_w, 
		file->out_h,
		BC_UYVA8888, 
		file->color_model,
		0,
		width,
		file->out_w);

	free(input_rows);

	return result;
}







static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int64_t offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v408_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int bytes = width * height * 4;
	int result = 0;
	unsigned char **output_rows;
	int i;
	quicktime_atom_t chunk_atom;
	if(!codec->work_buffer)
		codec->work_buffer = malloc(vtrack->track->tkhd.track_width * 
			vtrack->track->tkhd.track_height *
			4);


	output_rows = malloc(sizeof(unsigned char*) * height);
	for(i = 0; i < height; i++)
		output_rows[i] = codec->work_buffer + i * width * 4;

	cmodel_transfer(output_rows, 
		row_pointers,
		0,
		0,
		0,
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
		BC_UYVA8888, 
		0,
		width,
		width);

	quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, codec->work_buffer, bytes);
	quicktime_write_chunk_footer(file, 
		trak,
		vtrack->current_chunk,
		&chunk_atom, 
		1);

	vtrack->current_chunk++;
	
	free(output_rows);
	return result;
}

void quicktime_init_codec_v408(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_v408_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->fourcc = QUICKTIME_YUVA4444;
	codec_base->title = "Component Y'CbCrA 8-bit 4:4:4:4";
	codec_base->desc = "Component Y'CbCrA 8-bit 4:4:4:4";
}

