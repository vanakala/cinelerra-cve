#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "colormodels.h"
#include "funcprotos.h"
#include "jpeg.h"
#include "libmjpeg.h"
#include "quicktime.h"

// Jpeg types
#define JPEG_PROGRESSIVE 0
#define JPEG_MJPA 1
#define JPEG_MJPB 2

typedef struct
{
	unsigned char *buffer;
	long buffer_allocated;
	long buffer_size;
	mjpeg_t *mjpeg;
	int jpeg_type;
	unsigned char *temp_video;
} quicktime_jpeg_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int i;

	mjpeg_delete(codec->mjpeg);
	if(codec->buffer)
		free(codec->buffer);
	if(codec->temp_video)
		free(codec->temp_video);
	free(codec);
	return 0;
}

void quicktime_set_jpeg(quicktime_t *file, int quality, int use_float)
{
	int i;
	char *compressor;

	for(i = 0; i < file->total_vtracks; i++)
	{
		if(quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_JPEG) ||
			quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_MJPA) ||
			quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_RTJ0))
		{
			quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)file->vtracks[i].codec)->priv;
			mjpeg_set_quality(codec->mjpeg, quality);
			mjpeg_set_float(codec->mjpeg, use_float);
		}
	}
}

static int decode(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	mjpeg_t *mjpeg = codec->mjpeg;
	long size, field2_offset = 0;
	int track_height = trak->tkhd.track_height;
	int track_width = trak->tkhd.track_width;
	int result = 0;
	int field_dominance = trak->mdia.minf.stbl.stsd.table[0].field_dominance;

	mjpeg_set_cpus(codec->mjpeg, file->cpus);
	if(file->row_span) 
		mjpeg_set_rowspan(codec->mjpeg, file->row_span);
	else
		mjpeg_set_rowspan(codec->mjpeg, 0);

	quicktime_set_video_position(file, vtrack->current_position, track);
	size = quicktime_frame_size(file, vtrack->current_position, track);
	codec->buffer_size = size;

	if(size > codec->buffer_allocated)
	{
		codec->buffer_allocated = size;
		codec->buffer = realloc(codec->buffer, codec->buffer_allocated);
	}

	result = !quicktime_read_data(file, codec->buffer, size);
/*
 * printf("decode 1 %02x %02x %02x %02x %02x %02x %02x %02x\n", 
 * codec->buffer[0],
 * codec->buffer[1],
 * codec->buffer[2],
 * codec->buffer[3],
 * codec->buffer[4],
 * codec->buffer[5],
 * codec->buffer[6],
 * codec->buffer[7]
 * );
 */

	if(!result)
	{
		if(mjpeg_get_fields(mjpeg) == 2)
		{
			if(file->use_avi)
			{
				field2_offset = mjpeg_get_avi_field2(codec->buffer, 
					size, 
					&field_dominance);
			}
			else
			{
				field2_offset = mjpeg_get_quicktime_field2(codec->buffer, 
					size);
// Sanity check
				if(!field2_offset)
				{
					printf("decode: FYI field2_offset=0\n");
					field2_offset = mjpeg_get_field2(codec->buffer, size);
				}
			}
		}
		else
			field2_offset = 0;


//printf("decode 2 %d\n", field2_offset);
/*
 * printf("decode result=%d field1=%llx field2=%llx size=%d %02x %02x %02x %02x\n", 
 * result, 
 * quicktime_position(file) - size,
 * quicktime_position(file) - size + field2_offset,
 * size,
 * codec->buffer[0], 
 * codec->buffer[1], 
 * codec->buffer[field2_offset + 0], 
 * codec->buffer[field2_offset + 1]);
 */

 		if(file->in_x == 0 && 
			file->in_y == 0 && 
			file->in_w == track_width &&
			file->in_h == track_height &&
			file->out_w == track_width &&
			file->out_h == track_height)
		{
			int i;
			mjpeg_decompress(codec->mjpeg, 
				codec->buffer, 
				size,
				field2_offset,  
				row_pointers, 
				row_pointers[0], 
				row_pointers[1], 
				row_pointers[2],
				file->color_model,
				file->cpus);
		}
		else
		{
			int i;
			unsigned char **temp_rows;
			int temp_cmodel = BC_YUV888;
			int temp_rowsize = cmodel_calculate_pixelsize(temp_cmodel) * track_width;

			if(!codec->temp_video)
				codec->temp_video = malloc(temp_rowsize * track_height);
			temp_rows = malloc(sizeof(unsigned char*) * track_height);
			for(i = 0; i < track_height; i++)
				temp_rows[i] = codec->temp_video + i * temp_rowsize;

//printf("decode 10\n");
			mjpeg_decompress(codec->mjpeg, 
				codec->buffer, 
				size,
				field2_offset,  
				temp_rows, 
				temp_rows[0], 
				temp_rows[1], 
				temp_rows[2],
				temp_cmodel,
				file->cpus);

			cmodel_transfer(row_pointers, 
				temp_rows,
				row_pointers[0],
				row_pointers[1],
				row_pointers[2],
				temp_rows[0],
				temp_rows[1],
				temp_rows[2],
				file->in_x, 
				file->in_y, 
				file->in_w, 
				file->in_h,
				0, 
				0, 
				file->out_w, 
				file->out_h,
				temp_cmodel, 
				file->color_model,
				0,
				track_width,
				file->out_w);

//printf("decode 30\n");
			free(temp_rows);

//printf("decode 40\n");
		}
	}
//printf("decode 2 %d\n", result);

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int64_t offset = quicktime_position(file);
	int result = 0;
	long field2_offset;
	quicktime_atom_t chunk_atom;

//printf("encode 1\n");
	mjpeg_set_cpus(codec->mjpeg, file->cpus);

	mjpeg_compress(codec->mjpeg, 
		row_pointers, 
		row_pointers[0], 
		row_pointers[1], 
		row_pointers[2],
		file->color_model,
		file->cpus);
	if(codec->jpeg_type == JPEG_MJPA)
	{
		if(file->use_avi)
		{
			mjpeg_insert_avi_markers(&codec->mjpeg->output_data,
				&codec->mjpeg->output_size,
				&codec->mjpeg->output_allocated,
				2,
				&field2_offset);
		}
		else
		{
			mjpeg_insert_quicktime_markers(&codec->mjpeg->output_data,
				&codec->mjpeg->output_size,
				&codec->mjpeg->output_allocated,
				2,
				&field2_offset);
		}
	}

	quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, 
				mjpeg_output_buffer(codec->mjpeg), 
				mjpeg_output_size(codec->mjpeg));
	quicktime_write_chunk_footer(file, 
					trak,
					vtrack->current_chunk,
					&chunk_atom, 
					1);

	vtrack->current_chunk++;
//printf("encode 100\n");
	return result;
}

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

// Some JPEG_PROGRESSIVE is BC_YUV422P
	if(codec->jpeg_type == JPEG_PROGRESSIVE)
	{
		return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUV420P ||
			colormodel == BC_YUV422P ||
			colormodel == BC_YUV422);
	}
	else
	{
		return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
//			colormodel == BC_YUV420P ||
			colormodel == BC_YUV422P ||
			colormodel == BC_YUV422);
// The BC_YUV420P option was provided only for mpeg2movie use.because some 
// interlaced movies were accidentally in YUV4:2:0
	}
}

static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

	if(codec->jpeg_type == JPEG_PROGRESSIVE)
	{
		return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUV420P);
	}
	else
	{
		return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUV422P);
	}
}

static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;
	
	if(!strcasecmp(key, "jpeg_quality"))
	{
		mjpeg_set_quality(codec->mjpeg, *(int*)value);
	}
	else
	if(!strcasecmp(key, "jpeg_usefloat"))
	{
		mjpeg_set_float(codec->mjpeg, *(int*)value);
	}
	return 0;
}

static void init_codec_common(quicktime_video_map_t *vtrack, char *compressor)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_jpeg_codec_t *codec;
	int i, jpeg_type;

	if(quicktime_match_32(compressor, QUICKTIME_JPEG))
		jpeg_type = JPEG_PROGRESSIVE;
	if(quicktime_match_32(compressor, QUICKTIME_MJPA))
		jpeg_type = JPEG_MJPA;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_jpeg_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->set_parameter = set_parameter;
	codec_base->fourcc = compressor;
	codec_base->title = jpeg_type == JPEG_PROGRESSIVE ? "JPEG Photo" : "Motion JPEG A";
	codec_base->desc = codec_base->title;

/* Init private items */
	codec = codec_base->priv;
	codec->mjpeg = mjpeg_new(vtrack->track->tkhd.track_width, 
		vtrack->track->tkhd.track_height, 
		1 + (jpeg_type == JPEG_MJPA || jpeg_type == JPEG_MJPB));
	codec->jpeg_type = jpeg_type;

/* This information must be stored in the initialization routine because of */
/* direct copy rendering.  Quicktime for Windows must have this information. */
	if(jpeg_type == JPEG_MJPA && 
		!vtrack->track->mdia.minf.stbl.stsd.table[0].fields)
	{
		vtrack->track->mdia.minf.stbl.stsd.table[0].fields = 2;
		vtrack->track->mdia.minf.stbl.stsd.table[0].field_dominance = 1;
	}
}

void quicktime_init_codec_jpeg(quicktime_video_map_t *vtrack)
{
	init_codec_common(vtrack, QUICKTIME_JPEG);
}

void quicktime_init_codec_mjpa(quicktime_video_map_t *vtrack)
{
	init_codec_common(vtrack, QUICKTIME_MJPA);
}

void quicktime_init_codec_mjpg(quicktime_video_map_t *vtrack)
{
	init_codec_common(vtrack, "MJPG");
}





