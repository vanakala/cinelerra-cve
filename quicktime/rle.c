/* RLE codec */

#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"


typedef struct
{
	unsigned char *work_buffer;
	int buffer_size;
	unsigned char *output_temp;
} quicktime_rle_codec_t;


static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_rle_codec_t *codec;
	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
	if(codec->output_temp) free(codec->output_temp);
	free(codec);
}

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888);
}

static int source_cmodel(quicktime_t *file, int track)
{
	int depth = quicktime_video_depth(file, track);
	if(depth == 24) 
		return BC_RGB888;
	else
		return BC_ARGB8888;
}



static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_rle_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
 	int depth = quicktime_video_depth(file, track);
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int size;
	int result = 0;
	unsigned char *ptr;
	int start_line;
	int total_lines;
	int header;
	int row_bytes;
	int pixel_size;
	unsigned char *row_ptr;
	unsigned char *pixel;
	unsigned char *buffer_end;
	int code;
	int r, g, b, i;
	int need_temp;
	unsigned char **temp_rows = malloc(sizeof(unsigned char*) * height);
	int cmodel = source_cmodel(file, track);
	int skip;

	quicktime_set_video_position(file, vtrack->current_position, track);
	size = quicktime_frame_size(file, vtrack->current_position, track);
	row_bytes = depth / 8 * width;
	pixel_size = depth / 8;

	if(size <= 8) return 0;
	if(codec->buffer_size < size && codec->work_buffer)
	{
		free(codec->work_buffer);
		codec->work_buffer = 0;
	}
	if(!codec->work_buffer)
	{
		codec->work_buffer = malloc(size);
		codec->buffer_size = size;
	}

	if(!quicktime_read_data(file, 
		codec->work_buffer, 
		size))
		result = -1;

	ptr = codec->work_buffer;
	buffer_end = ptr + size;

// Chunk size
	ptr += 4;

// Header
	header = (ptr[0] << 8) | ptr[1];
	ptr += 2;

// Incremental change
	if(header & 0x0008)
	{
		start_line = (ptr[0] << 8) | ptr[1];
		ptr += 4;
		total_lines = (ptr[0] << 8) | ptr[1];
		ptr += 4;
	}
	else
// Keyframe
	{
		start_line = 0;
		total_lines = height;
	}


	if(cmodel != file->color_model ||
		file->in_x != 0 ||
		file->in_y != 0 ||
		file->in_w != width ||
		file->in_h != height ||
		file->out_w != width ||
		file->out_h != height)
		need_temp = 1;

	if(need_temp)
	{
		if(!codec->output_temp)
			codec->output_temp = calloc(1, height * row_bytes);
		row_ptr = codec->output_temp + start_line * row_bytes;
		for(i = 0; i < height; i++)
			temp_rows[i] = codec->output_temp + i * row_bytes;
	}
	else
	{
		row_ptr = row_pointers[start_line];
		for(i = 0; i < height; i++)
			temp_rows[i] = row_pointers[i];
	}

	switch(depth)
	{
		case 24:
			while(total_lines--)
			{
				skip = *ptr++;
				pixel = row_ptr + (skip - 1) * pixel_size;

				while(ptr < buffer_end && 
					(code = (char)*ptr++) != -1)
				{
					if(code == 0)
					{
// Skip code
						pixel += (*ptr++ - 1) * pixel_size;
					}
					else
// Run length encoded
					if(code < 0)
					{
						code *= -1;
						r = *ptr++;
						g = *ptr++;
						b = *ptr++;
						while(code--)
						{
							*pixel++ = r;
							*pixel++ = g;
							*pixel++ = b;
						}
					}
					else
// Uncompressed
					{
						while(code--)
						{
							*pixel++ = *ptr++;
							*pixel++ = *ptr++;
							*pixel++ = *ptr++;
						}
					}		
				}


				row_ptr += row_bytes;
			}
			break;
	}


	if(need_temp)
	{
		cmodel_transfer(row_pointers, 
			temp_rows,
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
			cmodel, 
			file->color_model,
			0,
			width,
			file->out_w);
	}

	free(temp_rows);

	return 0;
}



void quicktime_init_codec_rle(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_rle_codec_t *codec;
	codec_base->priv = calloc(1, sizeof(quicktime_rle_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->fourcc = "rle ";
	codec_base->title = "RLE";
	codec_base->desc = "Run length encoding";

	codec = (quicktime_rle_codec_t*)codec_base->priv;
}

