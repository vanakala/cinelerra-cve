/* 2002: Refurbished by Arthur Peters amep@softhome.net */
/* 2000: Original codec by Heroine Virtual */


#include "colormodels.h"
#include "funcprotos.h"
#include "libdv/dv.h"
#include "quicktime.h"

#include <pthread.h>
#include <string.h>

// Buffer sizes
#define DV_NTSC_SIZE 120000
#define DV_PAL_SIZE 144000

typedef struct
{
	dv_decoder_t *dv_decoder;
	dv_encoder_t *dv_encoder;
	unsigned char *data;
	unsigned char *temp_frame, **temp_rows;

	/* Parameters */
	int decode_quality;
	int anamorphic16x9;
	int vlc_encode_passes;
	int clamp_luma, clamp_chroma;

	int add_ntsc_setup;

	int rem_ntsc_setup;

	int parameters_changed;
} quicktime_dv_codec_t;

static pthread_mutex_t libdv_init_mutex = PTHREAD_MUTEX_INITIALIZER;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

	if(codec->dv_decoder)
	{
		dv_decoder_free( codec->dv_decoder );
		codec->dv_decoder = NULL;
	}
	
	if(codec->dv_encoder)
	{
		dv_encoder_free( codec->dv_encoder );
		codec->dv_encoder = NULL;
	}
	
	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->temp_rows) free(codec->temp_rows);
	free(codec->data);
	free(codec);
	return 0;
}

static int check_sequentiality( unsigned char **row_pointers,
								int bytes_per_row,
								int height )
{
	int i = 0;
 
	for(; i < height-1; i++)
	{
		if( row_pointers[i+1] - row_pointers[i] != bytes_per_row )
		{
			return 0;
		}
	}
	return 1;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	long bytes;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int result = 0;
	int i;
	int decode_colormodel = 0;
	int pitches[3] = { 720 * 2, 0, 0 };


	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	result = !quicktime_read_data(file, (char*)codec->data, bytes);

	if( codec->dv_decoder && codec->parameters_changed )
	{
		dv_decoder_free( codec->dv_decoder );
		codec->dv_decoder = NULL;
		codec->parameters_changed = 0;
	}
	
	if( ! codec->dv_decoder )
	{
		pthread_mutex_lock( &libdv_init_mutex );

			
		codec->dv_decoder = dv_decoder_new( codec->add_ntsc_setup,
											codec->clamp_luma,
											codec->clamp_chroma );
		codec->dv_decoder->prev_frame_decoded = 0;
		
		codec->parameters_changed = 0;
		pthread_mutex_unlock( &libdv_init_mutex );
	}

	if(codec->dv_decoder)
	{
		int is_sequential =
			check_sequentiality( row_pointers,
								 720 * cmodel_calculate_pixelsize(file->color_model),
								 file->out_h );

		codec->dv_decoder->quality = codec->decode_quality;

		dv_parse_header( codec->dv_decoder, codec->data );
		
// Libdv improperly decodes RGB colormodels.
		if((file->color_model == BC_YUV422 || 
			file->color_model == BC_RGB888) &&
			file->in_x == 0 && 
			file->in_y == 0 && 
			file->in_w == width &&
			file->in_h == height &&
			file->out_w == width &&
			file->out_h == height &&
			is_sequential)
		{
			if( file->color_model == BC_YUV422 )
			{
				pitches[0] = 720 * 2;
				dv_decode_full_frame( codec->dv_decoder, codec->data,
									  e_dv_color_yuv, row_pointers,
									  pitches );
			}
			else 
			if( file->color_model == BC_RGB888)
 			{
				pitches[0] = 720 * 3;
				dv_decode_full_frame( codec->dv_decoder, codec->data,
									  e_dv_color_rgb, row_pointers,
									  pitches );
			}
		}
		else
		{
			if(!codec->temp_frame)
			{
				codec->temp_frame = malloc(720 * 576 * 2);
				codec->temp_rows = malloc(sizeof(unsigned char*) * 576);
				for(i = 0; i < 576; i++)
					codec->temp_rows[i] = codec->temp_frame + 720 * 2 * i;
			}

		    decode_colormodel = BC_YUV422;
			pitches[0] = 720 * 2;
			dv_decode_full_frame( codec->dv_decoder, codec->data,
								  e_dv_color_yuv, codec->temp_rows,
								  pitches );
			



			cmodel_transfer(row_pointers, 
				codec->temp_rows,
				row_pointers[0],
				row_pointers[1],
				row_pointers[2],
				codec->temp_rows[0],
				codec->temp_rows[1],
				codec->temp_rows[2],
				file->in_x, 
				file->in_y, 
				file->in_w, 
				file->in_h,
				0, 
				0, 
				file->out_w, 
				file->out_h,
				decode_colormodel, 
				file->color_model,
				0,
				width,
				file->out_w);
		}
	}

//printf(__FUNCTION__ " 2\n");
	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int64_t offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = 720;
	int height_i = (height <= 480) ? 480 : 576;
	int i;
	unsigned char **input_rows;
	int is_pal = (height_i == 480) ? 0 : 1;
	int data_length = is_pal ? DV_PAL_SIZE : DV_NTSC_SIZE;
	int result = 0;
	int encode_colormodel = 0;
	dv_color_space_t encode_dv_colormodel = 0;
	quicktime_atom_t chunk_atom;

	if( codec->dv_encoder != NULL && codec->parameters_changed )
	{
		dv_encoder_free( codec->dv_encoder );
		codec->dv_encoder = NULL;
		codec->parameters_changed = 0;
	}
	
	if( ! codec->dv_encoder )
	{
		pthread_mutex_lock( &libdv_init_mutex );

		//printf( "dv.c encode: Alloc'ing encoder\n" );
	
		codec->dv_encoder = dv_encoder_new( codec->rem_ntsc_setup,
											codec->clamp_luma,
											codec->clamp_chroma );

		codec->parameters_changed = 0;
		pthread_mutex_unlock( &libdv_init_mutex );
	}

	if(codec->dv_encoder)
	{
		int is_sequential =
			check_sequentiality( row_pointers,
								 width_i * cmodel_calculate_pixelsize(file->color_model),
								 height );
	
		if( ( file->color_model == BC_YUV422
			  || file->color_model == BC_RGB888 ) &&
			width == width_i &&
			height == height_i &&
			is_sequential )
		{
			input_rows = row_pointers;
			encode_colormodel = file->color_model;
			switch( file->color_model )
			{
				case BC_YUV422:
					encode_dv_colormodel = e_dv_color_yuv;
//printf( "dv.c encode: e_dv_color_yuv\n" );
					break;
				case BC_RGB888:
					encode_dv_colormodel = e_dv_color_rgb;
//printf( "dv.c encode: e_dv_color_rgb\n" );
					break;
				default:
					return 0;
					break;
			}
		}
		else
		{
// The best colormodel for encoding is YUV 422

			if(!codec->temp_frame)
			{
				codec->temp_frame = malloc(720 * 576 * 2);
				codec->temp_rows = malloc(sizeof(unsigned char*) * 576);
				for(i = 0; i < 576; i++)
					codec->temp_rows[i] = codec->temp_frame + 720 * 2 * i;
			}
		
			cmodel_transfer(codec->temp_rows, /* Leave NULL if non existent */
							row_pointers,
							codec->temp_rows[0], /* Leave NULL if non existent */
							codec->temp_rows[1],
							codec->temp_rows[2],
							row_pointers[0], /* Leave NULL if non existent */
							row_pointers[1],
							row_pointers[2],
							0,   /* Dimensions to capture from input frame */
							0, 
							MIN(width, width_i), 
							MIN(height, height_i),
							0,   /* Dimensions to project on output frame */
							0, 
							MIN(width, width_i), 
							MIN(height, height_i),
							file->color_model, 
							BC_YUV422,
							0,    /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
							width,  /* For planar use the luma rowspan */
							width_i);


			input_rows = codec->temp_rows;
			encode_colormodel = BC_YUV422;
			encode_dv_colormodel = e_dv_color_yuv;
		}

// Setup the encoder
		codec->dv_encoder->is16x9 = codec->anamorphic16x9;
		codec->dv_encoder->vlc_encode_passes = codec->vlc_encode_passes;
		codec->dv_encoder->static_qno = 0;
		codec->dv_encoder->force_dct = DV_DCT_AUTO;
		codec->dv_encoder->isPAL = is_pal;


//printf("dv.c encode: 1 %d %d %d\n", width_i, height_i, encode_dv_colormodel);
		dv_encode_full_frame( codec->dv_encoder,
							  input_rows, encode_dv_colormodel, codec->data );
//printf("dv.c encode: 2 %d %d\n", width_i, height_i);

		quicktime_write_chunk_header(file, trak, &chunk_atom);
		result = !quicktime_write_data(file, codec->data, data_length);
		quicktime_write_chunk_footer(file, 
			trak,
			vtrack->current_chunk,
			&chunk_atom, 
			1);
		vtrack->current_chunk++;
//printf("encode 3\n");
	}

	return result;
}

// Logic: DV contains a mixture of 420 and 411 so can only
// output/input 444 or 422 and libdv can output/input RGB as well so
// we include that.

// This function is used as both reads_colormodel and writes_colormodel
static int colormodel_dv(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUV422);
}

static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;
	
	if(!strcasecmp(key, "dv_decode_quality"))
	{
		codec->decode_quality = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_anamorphic16x9"))
	{
		codec->anamorphic16x9 = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_vlc_encode_passes"))
	{
		codec->vlc_encode_passes = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_clamp_luma"))
	{
		codec->clamp_luma = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_clamp_chroma"))
	{
		codec->clamp_chroma = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_add_ntsc_setup"))
	{
		codec->add_ntsc_setup = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_rem_ntsc_setup"))
	{
		codec->rem_ntsc_setup = *(int*)value;
	}
	else
	{
		return 0;
	}

	codec->parameters_changed = 1;
	return 0;
}

static void init_codec_common(quicktime_video_map_t *vtrack, char *fourcc)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_dv_codec_t *codec;
	int i;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_dv_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = colormodel_dv;
	codec_base->writes_colormodel = colormodel_dv;
	codec_base->set_parameter = set_parameter;
	codec_base->fourcc = fourcc;
	codec_base->title = "DV";
	codec_base->desc = "DV";


	/* Init private items */

	codec = codec_base->priv;
	
	codec->dv_decoder = NULL;
	codec->dv_encoder = NULL;
	codec->decode_quality = DV_QUALITY_BEST;
	codec->anamorphic16x9 = 0;
	codec->vlc_encode_passes = 3;
	codec->clamp_luma = codec->clamp_chroma = 0;
	codec->add_ntsc_setup = 0;
	codec->parameters_changed = 0;

// Allocate extra to work around some libdv overrun
	codec->data = calloc(1, 144008);
}

void quicktime_init_codec_dv(quicktime_video_map_t *vtrack)
{
	init_codec_common(vtrack, QUICKTIME_DV);
}

void quicktime_init_codec_dvcp(quicktime_video_map_t *vtrack)
{
    init_codec_common(vtrack, QUICKTIME_DVCP);
}


void quicktime_init_codec_dv25(quicktime_video_map_t *vtrack)
{
	init_codec_common(vtrack, QUICKTIME_DV25);
}

void quicktime_init_codec_dvsd(quicktime_video_map_t *vtrack)
{
	init_codec_common(vtrack, QUICKTIME_DVSD);
}
