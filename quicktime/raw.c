#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "funcprotos.h"
#include "graphics.h"

typedef struct
{
	unsigned char *temp_frame;  /* For changing color models and scaling */
	unsigned char **temp_rows;
} quicktime_raw_codec_t;


static int quicktime_delete_codec_raw(quicktime_video_map_t *vtrack)
{
	quicktime_raw_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->temp_frame)
	{
		free(codec->temp_frame);
		free(codec->temp_rows);
	}
	free(codec);
	return 0;
}

static int source_cmodel(quicktime_t *file, int track)
{
	int depth = quicktime_video_depth(file, track);
	if(depth == 24) 
		return BC_RGB888;
	else
		return BC_ARGB8888;
}

static int quicktime_decode_raw(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	quicktime_trak_t *trak = file->vtracks[track].track;
	int frame_depth = quicktime_video_depth(file, track);
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	long bytes;
	int i;
	quicktime_raw_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;
	int pixel_size = frame_depth / 8;
	int cmodel = source_cmodel(file, track);
	int use_temp = (cmodel != file->color_model ||
		file->in_x != 0 ||
		file->in_y != 0 ||
		file->in_w != width ||
		file->in_h != height ||
		file->out_w != width ||
		file->out_h != height);
	unsigned char *temp_data;
	unsigned char **temp_rows = malloc(sizeof(unsigned char*) * height);

	if(use_temp)
	{
		if(!codec->temp_frame)
		{
			codec->temp_frame = malloc(cmodel_calculate_datasize(width, 
					height, 
					-1, 
					cmodel));
		}
		for(i = 0; i < height; i++)
			temp_rows[i] = codec->temp_frame + 
				cmodel_calculate_pixelsize(cmodel) * width * i;
		temp_data = codec->temp_frame;
	}
	else
	{
		temp_data = row_pointers[0];
		for(i = 0; i < height; i++)
			temp_rows[i] = row_pointers[i];
	}


/* Read data */
	quicktime_set_video_position(file, file->vtracks[track].current_position, track);
	bytes = quicktime_frame_size(file, file->vtracks[track].current_position, track);
	result = !quicktime_read_data(file, temp_data, bytes);

/* Convert colormodel */
	if(use_temp)
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

	return result;
}

static int quicktime_encode_raw(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_raw_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	longest offset = quicktime_position(file);
	int result = 0;
	register int i, j;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	int depth = quicktime_video_depth(file, track);
	longest bytes = height * width * depth / 8;
	longest bytes_per_row = width * depth / 8;
	unsigned char temp;
	int dest_cmodel;

//printf("quicktime_encode_raw %llx %llx\n", file->file_position, file->ftell_position);
	if(depth == 32)
	{
		dest_cmodel = BC_ARGB8888;

	}
	else
	{
		dest_cmodel = BC_RGB888;
	}




	if(file->color_model != dest_cmodel)
	{
		if(!codec->temp_frame)
		{
			codec->temp_frame = malloc(cmodel_calculate_datasize(width,
				height,
				-1,
				dest_cmodel));
			codec->temp_rows = malloc(sizeof(unsigned char*) * height);
			
			for(i = 0; i < height; i++)
			{
				codec->temp_rows[i] = codec->temp_frame + 
					i * cmodel_calculate_pixelsize(dest_cmodel) * width;
			}
		}
		
		
		
		cmodel_transfer(codec->temp_rows, /* Leave NULL if non existent */
			row_pointers,
			0, /* Leave NULL if non existent */
			0,
			0,
			row_pointers[0], /* Leave NULL if non existent */
			row_pointers[1],
			row_pointers[2],
			0,        /* Dimensions to capture from input frame */
			0, 
			width, 
			height,
			0,       /* Dimensions to project on output frame */
			0,
			width, 
			height,
			file->color_model, 
			dest_cmodel,
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			width,       /* For planar use the luma rowspan */
			width);     /* For planar use the luma rowspan */

		result = !quicktime_write_data(file, codec->temp_frame, 
			cmodel_calculate_datasize(width,
				height,
				-1,
				dest_cmodel));
	}
	else
	{
		result = !quicktime_write_data(file, row_pointers[0], 
			cmodel_calculate_datasize(width,
				height,
				-1,
				file->color_model));
	}





	quicktime_update_tables(file,
						file->vtracks[track].track,
						offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						bytes);

	file->vtracks[track].current_chunk++;
	return result;
}

int quicktime_raw_rows_consecutive(unsigned char **row_pointers, int w, int h, int depth)
{
	int i, result;
/* see if row_pointers are consecutive */
	for(i = 1, result = 1; i < h; i++)
	{
		if(row_pointers[i] - row_pointers[i - 1] != w * depth) result = 0;
	}
	return result;
}

static int quicktime_reads_colormodel_raw(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_BGR8888);
}

void quicktime_init_codec_raw(quicktime_video_map_t *vtrack)
{
	quicktime_raw_codec_t *priv;

	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_raw_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = quicktime_delete_codec_raw;
	((quicktime_codec_t*)vtrack->codec)->decode_video = quicktime_decode_raw;
	((quicktime_codec_t*)vtrack->codec)->encode_video = quicktime_encode_raw;
	((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = quicktime_reads_colormodel_raw;

	priv = ((quicktime_codec_t*)vtrack->codec)->priv;
}
