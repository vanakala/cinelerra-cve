/* General codec for all MPEG-4 derived encoding. */
/* Uses ffmpeg and encore50. */
/* Encore50 still seemed to provide better results than ffmpeg for encoding */
/* so it does all the generic MPEG-4 encoding. */





#include "avcodec.h"
#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "workarounds.h"
#include ENCORE_INCLUDE
#include DECORE_INCLUDE



#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#define FRAME_RATE_BASE 10000


typedef struct
{
#define FIELDS 2
	int decode_initialized[FIELDS];


// FFMpeg internals
    AVCodec *decoder[FIELDS];
	AVCodecContext *decoder_context[FIELDS];
    AVFrame picture[FIELDS];


// Decore internals
	DEC_PARAM dec_param[FIELDS];
	int decode_handle[FIELDS];

// Last frame decoded
	long last_frame[FIELDS];

	int got_key[FIELDS];










	int encode_initialized[FIELDS];
// Information for picking the right library routines.
// ID out of avcodec.h for the codec used.
// Invalid if encore50 is used.
	int ffmpeg_id;
// True if encore50 is being used.
	int use_encore;

// FFMpeg internals
    AVCodec *encoder[FIELDS];
	AVCodecContext *encoder_context[FIELDS];
	

// Encore internals
	int encode_handle[FIELDS];
	ENC_PARAM enc_param[FIELDS];
// Must count pframes in VBR
	int p_count[FIELDS];


// Encoding parameters
	int bitrate;
// For heroine 60 encoding, we want different streams for each field.
	int total_fields;
	long rc_period;          // the intended rate control averaging period
	long rc_reaction_period; // the reation period for rate control
	long rc_reaction_ratio;  // the ratio for down/up rate control
	long max_key_interval;   // the maximum interval between key frames
	int bitrate_tolerance;
	int interlaced;
	int gop_size;
	int max_quantizer;       // the upper limit of the quantizer
	int min_quantizer;       // the lower limit of the quantizer
	int quantizer;           // For vbr
	int quality;             // the forward search range for motion estimation
	int fix_bitrate;
	int use_deblocking;


// Temporary storage for color conversions
	char *temp_frame;
// Storage of compressed data
	unsigned char *work_buffer;
// Allocation of work_buffer
	int buffer_size;
} quicktime_mpeg4_codec_t;

int ffmpeg_initialized = 0;
pthread_mutex_t ffmpeg_lock = PTHREAD_MUTEX_INITIALIZER;


// Decore needs the user to specify handles
static int decode_handle = 1;
static int encode_handle = 0;









// Utilities for programs wishing to parse MPEG-4


// Determine of the compressed frame is a keyframe for direct copy
int quicktime_mpeg4_is_key(unsigned char *data, long size, char *codec_id)
{
	int result = 0;
	int i;

	if(quicktime_match_32(codec_id, QUICKTIME_DIVX) ||
		quicktime_match_32(codec_id, QUICKTIME_HV60))
	{
		for(i = 0; i < size - 5; i++)
		{
			if( data[i]     == 0x00 && 
				data[i + 1] == 0x00 &&
				data[i + 2] == 0x01 &&
				data[i + 3] == 0xb6)
			{
				if((data[i + 4] & 0xc0) == 0x0) 
					return 1;
				else
					return 0;
			}
		}
	}
	return result;
}


// Test for VOL header in frame
int quicktime_mpeg4_has_vol(unsigned char *data)
{
	if( data[0] == 0x00 &&
		data[1] == 0x00 &&
		data[2] == 0x01 &&
		data[3] == 0x00 &&
		data[4] == 0x00 &&
		data[5] == 0x00 &&
		data[6] == 0x01 &&
		data[7] == 0x20)
		return 1;
	else
		return 0;
}




static void putbits(unsigned char **data, 
	int *bit_pos, 
	uint64_t *bit_store, 
	int *total, 
	int count, 
	uint64_t value)
{
	value &= 0xffffffffffffffff >> (64 - count);

	while(64 - *bit_pos < count)
	{
		*(*data)++ = (*bit_store) >> 56;
		(*bit_store) <<= 8;
		(*bit_pos) -= 8;
	}

	(*bit_store) |= value << (64 - count - *bit_pos);
	(*bit_pos) += count;
	(*total) += count;
}


static void flushbits(unsigned char **data, 
	int *bit_pos, 
	uint64_t *bit_store)
{
//printf("flushbits %llx\n", (*bit_store));
	while((*bit_pos) > 0)
	{
		*(*data)++ = (*bit_store) >> 56;
		(*bit_store) <<= 8;
		(*bit_pos) -= 8;
	}
}




#define VO_START_CODE 		0x8      
#define VO_START_CODE_LENGTH	27
#define VOL_START_CODE 0x12             /* 25-MAR-97 JDL : according to WD2 */
#define VOL_START_CODE_LENGTH 28



int quicktime_mpeg4_write_vol(unsigned char *data_start,
	int vol_width, 
	int vol_height, 
	int time_increment_resolution, 
	double frame_rate)
{
	int written = 0;
	int bits, fixed_vop_time_increment;
	unsigned char *data = data_start;
	int bit_pos;
	uint64_t bit_store;
	int i, j;

	bit_store = 0;
	bit_pos = 0;
	vol_width = (int)((float)vol_width / 16 + 0.5) * 16;
	vol_height = (int)((float)vol_height / 16 + 0.5) * 16;


	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		VO_START_CODE_LENGTH, VO_START_CODE);
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		5, 0);				/* vo_id = 0								*/

	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		VOL_START_CODE_LENGTH, VOL_START_CODE);




	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		4, 0);				/* vol_id = 0								*/

	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 0);				/* random_accessible_vol = 0				*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		8, 1);				/* video_object_type_indication = 1 video	*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* is_object_layer_identifier = 1			*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		4, 2);				/* visual_object_layer_ver_id = 2			*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		3, 1);				/* visual_object_layer_priority = 1			*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		4, 1);				/* aspect_ratio_info = 1					*/







	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 0);				/* vol_control_parameter = 0				*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		2, 0);				/* vol_shape = 0 rectangular				*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* marker									*/







	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		16, time_increment_resolution);
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* marker									*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* fixed_vop_rate = 1						*/


	bits = 1;
	while((1 << bits) < time_increment_resolution) bits++;

// Log calculation fails for some reason
//	bits = (int)ceil(log((double)time_increment_resolution) / log(2.0));
//    if (bits < 1) bits=1;

	fixed_vop_time_increment = 
		(int)(time_increment_resolution / frame_rate + 0.1);

	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		bits, fixed_vop_time_increment);

	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* marker									*/

	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		13, vol_width);
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* marker									*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		13, vol_height);
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* marker									*/

	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 0);				/* interlaced = 0							*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* OBMC_disabled = 1						*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		2, 0);				/* vol_sprite_usage = 0						*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 0);				/* not_8_bit = 0							*/

	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 0);				/* vol_quant_type = 0						*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 0);				/* vol_quarter_pixel = 0					*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* complexity_estimation_disabled = 1		*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 1);				/* resync_marker_disabled = 1				*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 0);				/* data_partitioning_enabled = 0			*/
	putbits(&data, 
		&bit_pos, 
		&bit_store, 
		&written,
		1, 0);				/* scalability = 0							*/

	flushbits(&data, 
		&bit_pos,
		&bit_store);



/*
 * for(i = 0; i < data - data_start; i++)
 * 	for(j = 0x80; j >= 1; j /= 2)
 * 		printf("%d", (data_start[i] & j) ? 1 : 0);
 * printf("\n");
 */



	return data - data_start;
}







// Mpeg-4 interface

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_YUV420P);
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
		colormodel == BC_YUVA16161616 ||
		colormodel == BC_YUV420P ||
		colormodel == BC_YUV422 ||
		colormodel == BC_COMPRESSED);
}


static int delete_decoder(quicktime_mpeg4_codec_t *codec, int field)
{
	if(codec->decode_initialized[field])
	{
		pthread_mutex_lock(&ffmpeg_lock);

    	avcodec_close(codec->decoder_context[field]);
		free(codec->decoder_context[field]);

		pthread_mutex_unlock(&ffmpeg_lock);
		codec->decode_initialized[field] = 0;
	}
}


static int init_decode(quicktime_mpeg4_codec_t *codec, 
	int current_field, 
	int width_i, 
	int height_i)
{

	if(!ffmpeg_initialized)
	{
		ffmpeg_initialized = 1;
  		avcodec_init();
		avcodec_register_all();
	}


	codec->decoder[current_field] = avcodec_find_decoder(codec->ffmpeg_id);
	if(!codec->decoder[current_field])
	{
		printf("init_decode: avcodec_find_decoder returned NULL.\n");
		return 1;
	}

	codec->decoder_context[current_field] = avcodec_alloc_context();
	codec->decoder_context[current_field]->width = width_i;
	codec->decoder_context[current_field]->height = height_i;
	if(avcodec_open(codec->decoder_context[current_field], 
		codec->decoder[current_field]) < 0)
	{
		printf("init_decode: avcodec_open failed.\n");
	}
	return 0;
}



static int decode_wrapper(quicktime_t *file,
	quicktime_video_map_t *vtrack,
	quicktime_mpeg4_codec_t *codec,
	int frame_number, 
	int current_field, 
	int track)
{
	int got_picture = 0; 
	int result = 0; 
	int bytes;
 	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_trak_t *trak = vtrack->track;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = (int)((float)width / 16 + 0.5) * 16;
	int height_i = (int)((float)height / 16 + 0.5) * 16;

//printf("decode_wrapper 1\n");
	quicktime_set_video_position(file, frame_number, track); 
 
	bytes = quicktime_frame_size(file, frame_number, track); 

	if(!codec->work_buffer || codec->buffer_size < bytes) 
	{ 
		if(codec->work_buffer) free(codec->work_buffer); 
		codec->buffer_size = bytes; 
		codec->work_buffer = calloc(1, codec->buffer_size + 100); 
	} 
 
//printf("decode_wrapper 1 %d %llx %x\n", codec->ffmpeg_id, quicktime_position(file), bytes);
	if(!quicktime_read_data(file, 
		codec->work_buffer, 
		bytes))
		result = -1;
 
 
	if(!result) 
	{ 
/*
 * 		if(!codec->got_key[current_field])
 * 		{
 * 			if(!quicktime_mpeg4_is_key(codec->work_buffer, bytes, compressor)) 
 * 				return -1; 
 * 			else 
 * 			{
 * 				codec->got_key[current_field] = 1;
 * 			}
 * 		}
 */

//printf("decode_wrapper 2 %d\n", frame_number);

// No way to determine if there was an error based on nonzero status.
// Need to test row pointers to determine if an error occurred.
		result = avcodec_decode_video(codec->decoder_context[current_field], 
			&codec->picture[current_field], 
			&got_picture, 
			codec->work_buffer, 
			bytes);
//printf("decode_wrapper 3\n");
		if(codec->picture[current_field].data[0])
		{
			if(!codec->got_key[current_field])
			{
				codec->got_key[current_field] = 1;
			}
			result = 0;
		}
		else
		{
// ffmpeg can't recover if the first frame errored out, like in a direct copy
// sequence.
/*
 * 			delete_decoder(codec, current_field);
 * 			init_decode(codec, current_field, width_i, height_i);
 */
			result = 1;
		}

#ifdef ARCH_X86
		asm("emms");
#endif
	}
//printf("decode_wrapper 4\n");

	return result;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int i;
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_mpeg4_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = (int)((float)width / 16 + 0.5) * 16;
	int height_i = (int)((float)height / 16 + 0.5) * 16;
	int use_temp = 0;
	int input_cmodel;
	int current_field = vtrack->current_position % codec->total_fields;
	unsigned char **input_rows;
	int seeking_done = 0;


	pthread_mutex_lock(&ffmpeg_lock);


	if(!codec->decode_initialized[current_field])
	{
		init_decode(codec, current_field, width_i, height_i);
// Must decode frame with VOL header first but only the first frame in the
// field sequence has a VOL header.
		result = decode_wrapper(file, 
			vtrack, 
			codec, 
			current_field, 
			current_field, 
			track);
		codec->decode_initialized[current_field] = 1;
	}
//printf("decode 1\n");

// Handle seeking
	if(quicktime_has_keyframes(file, track) && 
		vtrack->current_position != codec->last_frame[current_field] + codec->total_fields)
	{
		int frame1, frame2 = vtrack->current_position, current_frame = frame2;
		int do_i_frame = 1;
//printf("decode 2\n");

// Get first keyframe of same field
		do
		{
			frame1 = quicktime_get_keyframe_before(file, 
				current_frame--, 
				track);
		}while(frame1 > 0 && (frame1 % codec->total_fields) != current_field);
//printf("decode 3\n");

// Keyframe is before last decoded frame and current frame is after last decoded
// frame, so instead of rerendering from the last keyframe we can rerender from
// the last decoded frame.
		if(frame1 < codec->last_frame[current_field] &&
			frame2 > codec->last_frame[current_field])
		{
//printf("decode 1 %d %d\n", frame1, frame2);
			frame1 = codec->last_frame[current_field] + codec->total_fields;
			do_i_frame = 0;
		}
//printf("decode 4\n");

		while(frame1 <= frame2)
		{
			result = decode_wrapper(file, 
				vtrack, 
				codec, 
				frame1, 
				current_field, 
				track);


// May need to do the first I frame twice.
			if(do_i_frame)
			{
				result = decode_wrapper(file, 
					vtrack, 
					codec, 
					frame1, 
					current_field, 
					track);
				do_i_frame = 0;
			}
			frame1 += codec->total_fields;
		}
//printf("decode 5\n");

		vtrack->current_position = frame2;
		seeking_done = 1;
	}
//printf("decode 6\n");

	if(!seeking_done)
	{
		result = decode_wrapper(file, 
			vtrack, 
			codec, 
			vtrack->current_position, 
			current_field, 
			track);
	}
	pthread_mutex_unlock(&ffmpeg_lock);
//printf("decode 10\n");


	codec->last_frame[current_field] = vtrack->current_position;









//	result = (result != 0);
	switch(codec->decoder_context[current_field]->pix_fmt)
	{
		case PIX_FMT_YUV420P:
			input_cmodel = BC_YUV420P;
			break;
		case PIX_FMT_YUV422:
			input_cmodel = BC_YUV422;
			break;
		case PIX_FMT_YUV422P:
			input_cmodel = BC_YUV422P;
			break;
	}


//printf("decode 20 %d %p\n", result, codec->picture[current_field].data[0]);
	if(codec->picture[current_field].data[0])
	{
		int y_out_size = codec->decoder_context[current_field]->width * 
			codec->decoder_context[current_field]->height;
		int u_out_size = codec->decoder_context[current_field]->width * 
			codec->decoder_context[current_field]->height / 
			4;
		int v_out_size = codec->decoder_context[current_field]->width * 
			codec->decoder_context[current_field]->height / 
			4;
		int y_in_size = codec->picture[current_field].linesize[0] * 
			codec->decoder_context[current_field]->height;
		int u_in_size = codec->picture[current_field].linesize[1] * 
			codec->decoder_context[current_field]->height / 
			4;
		int v_in_size = codec->picture[current_field].linesize[2] * 
			codec->decoder_context[current_field]->height / 
			4;
		input_rows = 
			malloc(sizeof(unsigned char*) * 
			codec->decoder_context[current_field]->height);

		for(i = 0; i < codec->decoder_context[current_field]->height; i++)
			input_rows[i] = codec->picture[current_field].data[0] + 
				i * 
				codec->decoder_context[current_field]->width * 
				cmodel_calculate_pixelsize(input_cmodel);

		if(!codec->temp_frame)
		{
			codec->temp_frame = malloc(y_out_size +
				u_out_size +
				v_out_size);
		}

		if(codec->picture[current_field].data[0])

			for(i = 0; i < codec->decoder_context[current_field]->height; i++)
			{
				memcpy(codec->temp_frame + i * codec->decoder_context[current_field]->width,
					codec->picture[current_field].data[0] + i * codec->picture[current_field].linesize[0],
					codec->decoder_context[current_field]->width);
			}

			for(i = 0; i < codec->decoder_context[current_field]->height; i += 2)
			{
				memcpy(codec->temp_frame + 
						y_out_size + 
						i / 2 * 
						codec->decoder_context[current_field]->width / 2,
					codec->picture[current_field].data[1] + 
						i / 2 * 
						codec->picture[current_field].linesize[1],
					codec->decoder_context[current_field]->width / 2);

				memcpy(codec->temp_frame + 
						y_out_size + 
						u_out_size + 
						i / 2 * 
						codec->decoder_context[current_field]->width / 2,
					codec->picture[current_field].data[2] + 
						i / 2 * 
						codec->picture[current_field].linesize[2],
					codec->decoder_context[current_field]->width / 2);
			}

			cmodel_transfer(row_pointers, /* Leave NULL if non existent */
				input_rows,
				row_pointers[0], /* Leave NULL if non existent */
				row_pointers[1],
				row_pointers[2],
				codec->temp_frame, /* Leave NULL if non existent */
				codec->temp_frame + y_out_size,
				codec->temp_frame + y_out_size + u_out_size,
				file->in_x,        /* Dimensions to capture from input frame */
				file->in_y, 
				file->in_w, 
				file->in_h,
				0,       /* Dimensions to project on output frame */
				0, 
				file->out_w, 
				file->out_h,
				input_cmodel, 
				file->color_model,
				0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
				codec->decoder_context[current_field]->width,       /* For planar use the luma rowspan */
				width);
		free(input_rows);
	}


//printf("decode 100\n");



	return result;
}


static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int64_t offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_mpeg4_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = (int)((float)width / 16 + 0.5) * 16;
	int height_i = (int)((float)height / 16 + 0.5) * 16;
	int result = 0;
	int i;
	int bytes = 0;
	int is_keyframe = 0;
	int current_field = vtrack->current_position % codec->total_fields;
	quicktime_atom_t chunk_atom;


//printf("encode 1\n");



	pthread_mutex_lock(&ffmpeg_lock);



	if(!codec->encode_initialized[current_field])
	{
// Encore section
		if(codec->ffmpeg_id == CODEC_ID_MPEG4)
		{
			codec->use_encore = 1;
			codec->encode_initialized[current_field] = 1;
			codec->encode_handle[current_field] = encode_handle++;
			codec->enc_param[current_field].x_dim = width_i;
			codec->enc_param[current_field].y_dim = height_i;
			codec->enc_param[current_field].framerate = 
				quicktime_frame_rate(file, track) / codec->total_fields;
			codec->enc_param[current_field].bitrate = 
				codec->bitrate / codec->total_fields;
			codec->enc_param[current_field].rc_period = codec->rc_period;
			codec->enc_param[current_field].rc_reaction_period = codec->rc_reaction_period;
			codec->enc_param[current_field].rc_reaction_ratio = codec->rc_reaction_ratio;
			codec->enc_param[current_field].max_quantizer = codec->max_quantizer;
			codec->enc_param[current_field].min_quantizer = codec->min_quantizer;
			codec->enc_param[current_field].max_key_interval = codec->max_key_interval;

			codec->enc_param[current_field].search_range = codec->quality * 3;
			if(codec->enc_param[current_field].search_range > 15)
				codec->enc_param[current_field].search_range = 15;

			encore(codec->encode_handle[current_field], 
				ENC_OPT_INIT, 
				&codec->enc_param[current_field], NULL);
			
		}
		else
// ffmpeg section
		{
			static char *video_rc_eq="tex^qComp";
			codec->encode_initialized[current_field] = 1;
			if(!ffmpeg_initialized)
			{
				ffmpeg_initialized = 1;
  				avcodec_init();
				avcodec_register_all();
			}

			codec->encoder[current_field] = avcodec_find_encoder(codec->ffmpeg_id);
			if(!codec->encoder[current_field])
			{
				printf("encode: avcodec_find_encoder returned NULL.\n");
				pthread_mutex_unlock(&ffmpeg_lock);
				return 1;
			}

			codec->encoder_context[current_field] = avcodec_alloc_context();
			codec->encoder_context[current_field]->frame_rate = FRAME_RATE_BASE *
				quicktime_frame_rate(file, track);
			codec->encoder_context[current_field]->width = width_i;
			codec->encoder_context[current_field]->height = height_i;
			codec->encoder_context[current_field]->gop_size = codec->gop_size;
			codec->encoder_context[current_field]->pix_fmt = PIX_FMT_YUV420P;
			codec->encoder_context[current_field]->bit_rate = codec->bitrate;
			codec->encoder_context[current_field]->bit_rate_tolerance = codec->bitrate_tolerance;
			codec->encoder_context[current_field]->rc_eq = video_rc_eq;
			codec->encoder_context[current_field]->qmin = 2;
			codec->encoder_context[current_field]->qmax = 31;
			codec->encoder_context[current_field]->max_qdiff = 3;
			codec->encoder_context[current_field]->qblur = 0.5;
			codec->encoder_context[current_field]->qcompress = 0.5;
			codec->encoder_context[current_field]->me_method = ME_FULL;

printf("encode %d %d %d %d %d\n", codec->gop_size, codec->bitrate, codec->bitrate_tolerance, codec->fix_bitrate, codec->interlaced);


			if(!codec->fix_bitrate)
			{
				codec->encoder_context[current_field]->flags |= CODEC_FLAG_QSCALE;
			}

			if(codec->interlaced)
			{
				codec->encoder_context[current_field]->flags |= CODEC_FLAG_INTERLACED_DCT;
			}

			avcodec_open(codec->encoder_context[current_field], codec->encoder[current_field]);

		}
	}


	if(!codec->work_buffer)
	{
		codec->buffer_size = width_i * height_i;
		codec->work_buffer = malloc(codec->buffer_size);
	}



// Encore section
	if(codec->use_encore)
	{
// Encore section
		ENC_FRAME encore_input;
		ENC_RESULT encore_result;


// Assume planes are contiguous.
// Encode directly from function arguments
		if(file->color_model == BC_YUV420P &&
			width == width_i &&
			height == height_i)
		{
			encore_input.image = row_pointers[0];
		}
// Convert to YUV420P
// Encode from temporary.
		else
		{
			if(!codec->temp_frame)
			{
				codec->temp_frame = malloc(width_i * height_i * 3 / 2);
			}

			cmodel_transfer(0, /* Leave NULL if non existent */
				row_pointers,
				codec->temp_frame, /* Leave NULL if non existent */
				codec->temp_frame + width_i * height_i,
				codec->temp_frame + width_i * height_i + width_i * height_i / 4,
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
				BC_YUV420P,
				0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
				width,       /* For planar use the luma rowspan */
				width_i);


			encore_input.image = codec->temp_frame;
		}



		bzero(codec->work_buffer, codec->buffer_size);
		encore_input.bitstream = codec->work_buffer;
		encore_input.length = 0;
		encore_input.quant = !codec->fix_bitrate ? codec->quantizer : 0;

		if(codec->p_count == 0)
		{
			codec->p_count[current_field]++;
		}
		else
		{
			codec->p_count[current_field]++;
			if(codec->p_count[current_field] >= codec->max_key_interval)
				codec->p_count[current_field] = 0;
		}

		encore(codec->encode_handle[current_field],	
			0,	
			&encore_input,
			&encore_result);

		bytes = encore_input.length;
		is_keyframe = encore_result.isKeyFrame;
	}
	else
// ffmpeg section
	{
		AVFrame pict_tmp;

		if(width_i == width && 
			height_i == height && 
			file->color_model == BC_YUV420P)
		{
			pict_tmp.data[0] = row_pointers[0];
			pict_tmp.data[1] = row_pointers[1];
			pict_tmp.data[2] = row_pointers[2];
			pict_tmp.linesize[0] = width_i;
			pict_tmp.linesize[1] = width_i / 2;
			pict_tmp.linesize[2] = width_i / 2;
		}
		else
		{
			if(!codec->temp_frame)
			{
				codec->temp_frame = malloc(width_i * height_i * 3 / 2);
			}

			cmodel_transfer(0, /* Leave NULL if non existent */
				row_pointers,
				codec->temp_frame, /* Leave NULL if non existent */
				codec->temp_frame + width_i * height_i,
				codec->temp_frame + width_i * height_i + width_i * height_i / 4,
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
				BC_YUV420P,
				0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
				width,       /* For planar use the luma rowspan */
				width_i);

			pict_tmp.data[0] = codec->temp_frame;
			pict_tmp.data[1] = codec->temp_frame + width_i * height_i;
			pict_tmp.data[2] = codec->temp_frame + width_i * height_i + width_i * height_i / 4;
			pict_tmp.linesize[0] = width_i;
			pict_tmp.linesize[1] = width_i / 2;
			pict_tmp.linesize[2] = width_i / 2;
		}


		if(codec->quantizer >= 0)
			pict_tmp.quality = codec->quantizer;
		bytes = avcodec_encode_video(codec->encoder_context[current_field], 
			codec->work_buffer, 
        	codec->buffer_size, 
        	&pict_tmp);
		is_keyframe = pict_tmp.key_frame;
	}






	pthread_mutex_unlock(&ffmpeg_lock);
	quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, 
		codec->work_buffer, 
		bytes);
	quicktime_write_chunk_footer(file, 
		trak,
		vtrack->current_chunk,
		&chunk_atom, 
		1);
	if(is_keyframe)
		quicktime_insert_keyframe(file, 
			vtrack->current_position, 
			track);
//printf("encode 10\n");

	vtrack->current_chunk++;
	return result;
}







static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;

	if(quicktime_match_32(compressor, QUICKTIME_DIVX) ||
		quicktime_match_32(compressor, QUICKTIME_HV60))
	{
		quicktime_mpeg4_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

		if(!strcasecmp(key, "divx_bitrate"))
			codec->bitrate = *(int*)value;
		else
		if(!strcasecmp(key, "divx_rc_period"))
			codec->rc_period = *(int*)value;
		else
		if(!strcasecmp(key, "divx_rc_reaction_ratio"))
			codec->rc_reaction_ratio = *(int*)value;
		else
		if(!strcasecmp(key, "divx_rc_reaction_period"))
			codec->rc_reaction_period = *(int*)value;
		else
		if(!strcasecmp(key, "divx_max_key_interval"))
			codec->max_key_interval = *(int*)value;
		else
		if(!strcasecmp(key, "divx_max_quantizer"))
			codec->max_quantizer = *(int*)value;
		else
		if(!strcasecmp(key, "divx_min_quantizer"))
			codec->min_quantizer = *(int*)value;
		else
		if(!strcasecmp(key, "divx_quantizer"))
			codec->quantizer = *(int*)value;
		else
		if(!strcasecmp(key, "divx_quality"))
			codec->quality = *(int*)value;
		else
		if(!strcasecmp(key, "divx_fix_bitrate"))
			codec->fix_bitrate = *(int*)value;
		else
		if(!strcasecmp(key, "divx_use_deblocking"))
			codec->use_deblocking = *(int*)value;
	}
	else
	if(quicktime_match_32(compressor, QUICKTIME_DIV3))
	{
		quicktime_mpeg4_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
		if(!strcasecmp(key, "div3_bitrate"))
			codec->bitrate = *(int*)value;
		else
		if(!strcasecmp(key, "div3_bitrate_tolerance"))
			codec->bitrate_tolerance = *(int*)value;
		else
		if(!strcasecmp(key, "div3_interlaced"))
			codec->interlaced = *(int*)value;
		else
		if(!strcasecmp(key, "div3_gop_size"))
			codec->gop_size = *(int*)value;
		else
		if(!strcasecmp(key, "div3_quantizer"))
			codec->quantizer = *(int*)value;
		else
		if(!strcasecmp(key, "div3_fix_bitrate"))
			codec->fix_bitrate = *(int*)value;
	}
	return 0;
}



static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *codec;
	int i;


	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	for(i = 0; i < codec->total_fields; i++)
	{
		if(codec->encode_initialized[i])
		{
			pthread_mutex_lock(&ffmpeg_lock);
			if(codec->use_encore)
			{
				encore(codec->encode_handle[i],
					ENC_OPT_RELEASE,
					0,
					0);
			}
			else
			{
    			avcodec_close(codec->encoder_context[i]);
				free(codec->encoder_context[i]);
			}
			pthread_mutex_unlock(&ffmpeg_lock);
		}
		delete_decoder(codec, i);
	}


	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->work_buffer) free(codec->work_buffer);


	free(codec);
	return 0;
}




static quicktime_mpeg4_codec_t* init_common(quicktime_video_map_t *vtrack, 
	char *compressor,
	char *title,
	char *description)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_mpeg4_codec_t *codec;

	codec_base->priv = calloc(1, sizeof(quicktime_mpeg4_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->set_parameter = set_parameter;
	codec_base->fourcc = compressor;
	codec_base->title = title;
	codec_base->desc = description;

	codec = (quicktime_mpeg4_codec_t*)codec_base->priv;



// Set defaults
	codec->bitrate = 1000000;
	codec->rc_period = 50;
	codec->rc_reaction_ratio = 45;
	codec->rc_reaction_period = 10;
	codec->max_key_interval = 45;
	codec->max_quantizer = 31;
	codec->min_quantizer = 1;
	codec->quantizer = 10;
	codec->quality = 5;
	codec->fix_bitrate = 1;
	codec->total_fields = 1;



	return codec;
}




// Mike Rowe Soft MPEG-4
void quicktime_init_codec_div3(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_DIV3,
		"DIVX",
		"Mike Row Soft MPEG4 Version 3");
	result->ffmpeg_id = CODEC_ID_MSMPEG4V3;
}

// Mike Rowe Soft MPEG-4
void quicktime_init_codec_div3lower(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_DIV3_LOWER,
		"DIVX",
		"Mike Row Soft MPEG4 Version 3");
	result->ffmpeg_id = CODEC_ID_MSMPEG4V3;
}


// Generic MPEG-4
void quicktime_init_codec_divx(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_DIVX,
		"MPEG4",
		"Generic MPEG Four");
	result->ffmpeg_id = CODEC_ID_MPEG4;
}


// Mormon MPEG-4
void quicktime_init_codec_svq1(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_SVQ1,
		"Sorenson Version 1",
		"From the chearch of codecs of yesterday's sights");
	result->ffmpeg_id = CODEC_ID_SVQ1;
}

// field based MPEG-4
void quicktime_init_codec_hv60(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_HV60,
		"Heroine 60",
		"MPEG 4 with alternating streams every other frame.  (Not standardized)");
	result->total_fields = 2;
	result->ffmpeg_id = CODEC_ID_MPEG4;
}






