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
//#include DECORE_INCLUDE



#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
//	DEC_PARAM dec_param[FIELDS];
//	int decode_handle[FIELDS];

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



// Decore needs the user to specify handles
static int decode_handle = 1;
static int encode_handle = 0;









// Direct copy routines


// Determine of the compressed frame is a keyframe for direct copy
int quicktime_mpeg4_is_key(unsigned char *data, long size, char *codec_id)
{
	int result = 0;
	int i;

	if(quicktime_match_32(codec_id, QUICKTIME_DIVX) ||
		quicktime_match_32(codec_id, QUICKTIME_MP4V) ||
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
	value &= 0xffffffffffffffffLL >> (64 - count);

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
	vol_width = quicktime_quantize16(vol_width);
	vol_height = quicktime_quantize16(vol_height);


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



// Create the header for the esds block which is used in mp4v.
// Taken from libavcodec
// Returns the size
static int write_mp4v_header(unsigned char *data,
	int w, 
	int h,
	double frame_rate)
{
	unsigned char *start = data;


/*
 * 	static unsigned char test[] = 
 * 	{
 * 
 * 		0x00, 0x00, 0x01, 0xb0, 0x01, 0x00, 0x00, 0x01, 0xb5, 0x89, 0x13,
 * 		0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x20, 0x00, 0xc4, 0x8d,
 * 		0x8a, 0xee, 0x05, 0x28, 0x04, 0x5a, 0x14, 0x63, 0x00, 0x00, 0x01, 0xb2,
 * 		0x46, 0x46, 0x6d, 0x70, 0x65, 0x67, 0x43, 0x56, 0x53, 0x62, 0x34, 0x37,
 * 		0x35, 0x38
 * 
 * 	};
 * 	memcpy(data, test, sizeof(test));
 * 
 * 	return sizeof(test);
 */

// From ffmpeg
// Advanced simple level 1
//	int profile_level = 0xf3;

	int profile_level = 0x1;
//	int vo_version_id = 5;
	int vo_version_id = 1;





// VOS startcode
	*data++ = 0x00;
	*data++ = 0x00;
	*data++ = 0x01;
	*data++ = 0xb0;
	*data++ = profile_level;

// Visual object startcode
	*data++ = 0x00;
	*data++ = 0x00;
	*data++ = 0x01;
	*data++ = 0xb5;
	*data++ = ((unsigned char)0x1 << 7) |
		((unsigned char)vo_version_id << 3) |
// Priority
		(unsigned char)1;
// visual object type video
	*data++ = (0x1 << 4) |
// Video signal type
		(0 << 3) |
// Stuffing
		0x3;

//	*data++ = 0x40;
//	*data++ = 0xc0;
//	*data++ = 0xcf;

// video object
	int vol_size = quicktime_mpeg4_write_vol(data,
		w, 
		h, 
		60000, 
		frame_rate);
	data += vol_size;

	return data - start;
}







// Mpeg-4 interface

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_codec_t *codec = (quicktime_codec_t*)vtrack->codec;
	return (colormodel == BC_YUV420P && 
		!quicktime_match_32(QUICKTIME_SVQ1, codec->fourcc));
}

static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return colormodel == BC_YUV420P;
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


static int init_decode(quicktime_t *file,
	quicktime_mpeg4_codec_t *codec, 
	quicktime_trak_t *trak,
	int current_field, 
	int width_i, 
	int height_i)
{
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
	quicktime_esds_t *esds = &stsd_table->esds;

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

	AVCodecContext *context = 
		codec->decoder_context[current_field] = 
		avcodec_alloc_context();
	context->width = width_i;
	context->height = height_i;


	if(esds->mpeg4_header && esds->mpeg4_header_size) 
	{
		context->extradata = esds->mpeg4_header;
		context->extradata_size = esds->mpeg4_header_size;
	}

	if(file->cpus > 1)
	{
		avcodec_thread_init(context, file->cpus);
		context->thread_count = file->cpus;
	}

	if(avcodec_open(context, 
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
	int track,
	int drop_it)
{

	int got_picture = 0; 
	int result = 0; 
	int bytes = 0;
	int header_bytes = 0;
 	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_trak_t *trak = vtrack->track;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i;
	int height_i;

	if(codec->ffmpeg_id == CODEC_ID_SVQ1)
	{
		width_i = quicktime_quantize32(width);
		height_i = quicktime_quantize32(height);
	}
	else
	{
		width_i = quicktime_quantize16(width);
		height_i = quicktime_quantize16(height);
	}


	quicktime_set_video_position(file, frame_number, track);
 
	bytes = quicktime_frame_size(file, frame_number, track); 
	if(frame_number == 0 && codec->ffmpeg_id == CODEC_ID_SVQ3 && stsd_table->extradata)
	{
		codec->decoder_context[current_field]->extradata_size = stsd_table->extradata_size;	
		codec->decoder_context[current_field]->extradata = stsd_table->extradata;
	} else
	if(frame_number == 0)
	{
		header_bytes = stsd_table->esds.mpeg4_header_size;
	}

	if(!codec->work_buffer || codec->buffer_size < bytes + header_bytes) 
	{ 
		if(codec->work_buffer) free(codec->work_buffer); 
		codec->buffer_size = bytes + header_bytes; 
		codec->work_buffer = calloc(1, codec->buffer_size + 100); 
	} 

	// Special handling of SVQ3 codec (some others might be alike)
 
	if(header_bytes)
		memcpy(codec->work_buffer, stsd_table->esds.mpeg4_header, header_bytes);

	if(!quicktime_read_data(file, 
		codec->work_buffer + header_bytes, 
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


// No way to determine if there was an error based on nonzero status.
// Need to test row pointers to determine if an error occurred.
		if(drop_it)
			codec->decoder_context[current_field]->skip_frame = AVDISCARD_NONREF;
		else
			codec->decoder_context[current_field]->skip_frame = AVDISCARD_DEFAULT;
		result = avcodec_decode_video(codec->decoder_context[current_field], 
			&codec->picture[current_field], 
			&got_picture, 
			codec->work_buffer, 
			bytes + header_bytes);



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
	int width_i;
	int height_i;
	int use_temp = 0;
	int input_cmodel;
	int current_field = vtrack->current_position % codec->total_fields;
	unsigned char **input_rows;
	int seeking_done = 0;

	if(codec->ffmpeg_id == CODEC_ID_SVQ1)
	{
		width_i = quicktime_quantize32(width);
		height_i = quicktime_quantize32(height);
	}
	else
	{
		width_i = quicktime_quantize16(width);
		height_i = quicktime_quantize16(height);
	}

	pthread_mutex_lock(&ffmpeg_lock);


	if(!codec->decode_initialized[current_field])
	{
		int current_frame = vtrack->current_position;
		init_decode(file, codec, trak, current_field, width_i, height_i);
// Must decode frame with stream header first but only the first frame in the
// field sequence has a stream header.
		result = decode_wrapper(file, 
			vtrack, 
			codec, 
			current_field, 
			current_field, 
			track,
			0);
// Reset position because decode wrapper set it
		quicktime_set_video_position(file, current_frame, track);
		codec->decode_initialized[current_field] = 1;
	}

// Handle seeking
	if(quicktime_has_keyframes(file, track) && 
		vtrack->current_position != codec->last_frame[current_field] + codec->total_fields)
	{
		int frame1, frame2 = vtrack->current_position, current_frame = frame2;
		int do_i_frame = 1;

// Get first keyframe of same field
		do
		{
			frame1 = quicktime_get_keyframe_before(file, 
				current_frame--, 
				track);
		}while(frame1 > 0 && (frame1 % codec->total_fields) != current_field);

// Keyframe is before last decoded frame and current frame is after last decoded
// frame, so instead of rerendering from the last keyframe we can rerender from
// the last decoded frame.
		if(frame1 < codec->last_frame[current_field] &&
			frame2 > codec->last_frame[current_field])
		{
			frame1 = codec->last_frame[current_field] + codec->total_fields;
			do_i_frame = 0;
		}

		while(frame1 <= frame2)
		{
			result = decode_wrapper(file, 
				vtrack, 
				codec, 
				frame1, 
				current_field, 
				track,
				(frame1 < frame2));


// May need to do the first I frame twice.
			if(do_i_frame)
			{
				result = decode_wrapper(file, 
					vtrack, 
					codec, 
					frame1, 
					current_field, 
					track,
					0);
				do_i_frame = 0;
			}
			frame1 += codec->total_fields;
		}

		vtrack->current_position = frame2;
		seeking_done = 1;
	}


	if(!seeking_done)
	{
		result = decode_wrapper(file, 
			vtrack, 
			codec, 
			vtrack->current_position, 
			current_field, 
			track,
			0);
	}
	pthread_mutex_unlock(&ffmpeg_lock);



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
		case PIX_FMT_YUV410P:
			input_cmodel = BC_YUV9P;
			break;
		default:
			fprintf(stderr, 
				"mpeg4 decode: unrecognized color model %d\n", 
				codec->decoder_context[current_field]->pix_fmt);
			input_cmodel = BC_YUV420P;
			break;
	}





	if(codec->picture[current_field].data[0])
	{

		input_rows = 
			malloc(sizeof(unsigned char*) * 
			codec->decoder_context[current_field]->height);


		for(i = 0; i < codec->decoder_context[current_field]->height; i++)
			input_rows[i] = codec->picture[current_field].data[0] + 
				i * 
				codec->decoder_context[current_field]->width * 
				cmodel_calculate_pixelsize(input_cmodel);


		cmodel_transfer(row_pointers, /* Leave NULL if non existent */
			input_rows,
			row_pointers[0], /* Leave NULL if non existent */
			row_pointers[1],
			row_pointers[2],
			codec->picture[current_field].data[0], /* Leave NULL if non existent */
			codec->picture[current_field].data[1],
			codec->picture[current_field].data[2],
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
			codec->picture[current_field].linesize[0],       /* For planar use the luma rowspan */
			width);
		free(input_rows);
	}




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
	int width_i = quicktime_quantize16(width);
	int height_i = quicktime_quantize16(height);
	int result = 0;
	int i;
	int bytes = 0;
	int is_keyframe = 0;
	int current_field = vtrack->current_position % codec->total_fields;
	quicktime_atom_t chunk_atom;





	pthread_mutex_lock(&ffmpeg_lock);



	if(!codec->encode_initialized[current_field])
	{
// Encore section
		if(codec->ffmpeg_id == CODEC_ID_MPEG4 && codec->use_encore)
		{
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
			AVCodecContext *context = codec->encoder_context[current_field];

			context->width = width_i;
			context->height = height_i;
			context->gop_size = codec->gop_size;
			context->pix_fmt = PIX_FMT_YUV420P;
			context->bit_rate = codec->bitrate / codec->total_fields;
			context->bit_rate_tolerance = codec->bitrate_tolerance;
			context->rc_eq = video_rc_eq;
        	context->rc_max_rate = 0;
        	context->rc_min_rate = 0;
        	context->rc_buffer_size = 0;
			context->qmin = 
				(!codec->fix_bitrate ? codec->quantizer : 2);
			context->qmax = 
				(!codec->fix_bitrate ? codec->quantizer : 31);
			context->lmin = 2 * FF_QP2LAMBDA;
			context->lmax = 31 * FF_QP2LAMBDA;
			context->mb_lmin = 2 * FF_QP2LAMBDA;
			context->mb_lmax = 31 * FF_QP2LAMBDA;
			context->max_qdiff = 3;
			context->qblur = 0.5;
			context->qcompress = 0.5;
// It needs the time per frame, not the frame rate.
			context->time_base.den = quicktime_frame_rate_n(file, track);
			context->time_base.num = quicktime_frame_rate_d(file, track);

        	context->b_quant_factor = 1.25;
        	context->b_quant_offset = 1.25;
			context->error_resilience = FF_ER_CAREFULL;
			context->error_concealment = 3;
			context->frame_skip_cmp = FF_CMP_DCTMAX;
			context->ildct_cmp = FF_CMP_VSAD;
			context->intra_dc_precision = 0;
        	context->intra_quant_bias = FF_DEFAULT_QUANT_BIAS;
        	context->inter_quant_bias = FF_DEFAULT_QUANT_BIAS;
        	context->i_quant_factor = -0.8;
        	context->i_quant_offset = 0.0;
			context->mb_decision = FF_MB_DECISION_SIMPLE;
			context->mb_cmp = FF_CMP_SAD;
			context->me_sub_cmp = FF_CMP_SAD;
			context->me_cmp = FF_CMP_SAD;
			context->me_pre_cmp = FF_CMP_SAD;
			context->me_method = ME_EPZS;
			context->me_subpel_quality = 8;
			context->me_penalty_compensation = 256;
			context->me_range = 0;
			context->me_threshold = 0;
			context->mb_threshold = 0;
			context->nsse_weight= 8;
        	context->profile= FF_PROFILE_UNKNOWN;
			context->rc_buffer_aggressivity = 1.0;
        	context->level= FF_LEVEL_UNKNOWN;
			context->flags |= CODEC_FLAG_H263P_UMV;
			context->flags |= CODEC_FLAG_AC_PRED;
			context->flags |= CODEC_FLAG_4MV;
// Not compatible with Win
//			context->flags |= CODEC_FLAG_QPEL;

			if(file->cpus > 1)
			{
				avcodec_thread_init(context, file->cpus);
				context->thread_count = file->cpus;
			}

			if(!codec->fix_bitrate)
				context->flags |= CODEC_FLAG_QSCALE;

			if(codec->interlaced)
			{
				context->flags |= CODEC_FLAG_INTERLACED_DCT;
				context->flags |= CODEC_FLAG_INTERLACED_ME;
			}


/*
 * printf("encode gop_size=%d fix_bitrate=%d quantizer=%d\n", 
 * codec->gop_size,
 * codec->fix_bitrate,
 * codec->quantizer);
 */
			avcodec_open(context, codec->encoder[current_field]);

   			avcodec_get_frame_defaults(&codec->picture[current_field]);

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
		AVCodecContext *context = codec->encoder_context[current_field];
		AVFrame *picture = &codec->picture[current_field];

		if(width_i == width && 
			height_i == height && 
			file->color_model == BC_YUV420P)
		{
			picture->data[0] = row_pointers[0];
			picture->data[1] = row_pointers[1];
			picture->data[2] = row_pointers[2];
			picture->linesize[0] = width_i;
			picture->linesize[1] = width_i / 2;
			picture->linesize[2] = width_i / 2;
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

			picture->data[0] = codec->temp_frame;
			picture->data[1] = codec->temp_frame + width_i * height_i;
			picture->data[2] = codec->temp_frame + width_i * height_i + width_i * height_i / 4;
			picture->linesize[0] = width_i;
			picture->linesize[1] = width_i / 2;
			picture->linesize[2] = width_i / 2;
		}


		picture->pict_type = 0;
		picture->quality = 0;
		picture->pts = vtrack->current_position * quicktime_frame_rate_d(file, track);
		picture->key_frame = 0;
		bytes = avcodec_encode_video(context, 
			codec->work_buffer, 
        	codec->buffer_size, 
        	picture);
		is_keyframe = context->coded_frame && context->coded_frame->key_frame;
/*
 * printf("encode current_position=%d is_keyframe=%d\n", 
 * vtrack->current_position,
 * is_keyframe);
 */

		if(!trak->mdia.minf.stbl.stsd.table[0].esds.mpeg4_header_size &&
			!strcmp(((quicktime_codec_t*)vtrack->codec)->fourcc, QUICKTIME_MP4V))
		{
			unsigned char temp[1024];
			unsigned char *ptr = temp;
			for(i = 0; i < bytes - 4; i++)
			{
				if(!(codec->work_buffer[i] == 0x00 &&
					codec->work_buffer[i + 1] == 0x00 &&
					codec->work_buffer[i + 2] == 0x01 &&
					codec->work_buffer[i + 3] == 0xb3))
				{
					*ptr++ = codec->work_buffer[i];
				}
				else
					break;
			}
			quicktime_set_mpeg4_header(&trak->mdia.minf.stbl.stsd.table[0],
				temp, 
				ptr - temp);
			trak->mdia.minf.stbl.stsd.table[0].version = 0;
		}
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
	if(is_keyframe || vtrack->current_position == 0)
		quicktime_insert_keyframe(file, 
			vtrack->current_position, 
			track);

	vtrack->current_chunk++;
	return result;
}






static void flush(quicktime_t *file, int track)
{
	quicktime_video_map_t *track_map = &(file->vtracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_mpeg4_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

// Create header
	if(!trak->mdia.minf.stbl.stsd.table[0].esds.mpeg4_header_size &&
		!strcmp(((quicktime_codec_t*)track_map->codec)->fourcc, QUICKTIME_MP4V))
	{
		int width = trak->tkhd.track_width;
		int height = trak->tkhd.track_height;
		int width_i = quicktime_quantize16(width);
		int height_i = quicktime_quantize16(height);

		unsigned char temp[1024];
		int size = write_mp4v_header(temp,
			width_i,
			height_i,
			quicktime_frame_rate(file, track));
		quicktime_set_mpeg4_header(&trak->mdia.minf.stbl.stsd.table[0],
			temp, 
			size);
	}

// Create udta
	file->moov.udta.require = strdup("QuickTime 6.0 or greater");
	file->moov.udta.require_len = strlen(file->moov.udta.require);
}








static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;

	if(quicktime_match_32(compressor, QUICKTIME_DIVX) ||
		quicktime_match_32(compressor, QUICKTIME_MP42) ||
		quicktime_match_32(compressor, QUICKTIME_MPG4) ||
		quicktime_match_32(compressor, QUICKTIME_DX50) ||
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
	if(quicktime_match_32(compressor, QUICKTIME_DIV3) ||
		quicktime_match_32(compressor, QUICKTIME_MP4V))
	{
		quicktime_mpeg4_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
		if(!strcasecmp(key, "ffmpeg_bitrate"))
			codec->bitrate = *(int*)value;
		else
		if(!strcasecmp(key, "ffmpeg_bitrate_tolerance"))
			codec->bitrate_tolerance = *(int*)value;
		else
		if(!strcasecmp(key, "ffmpeg_interlaced"))
			codec->interlaced = *(int*)value;
		else
		if(!strcasecmp(key, "ffmpeg_gop_size"))
			codec->gop_size = *(int*)value;
		else
		if(!strcasecmp(key, "ffmpeg_quantizer"))
			codec->quantizer = *(int*)value;
		else
		if(!strcasecmp(key, "ffmpeg_fix_bitrate"))
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
	codec_base->flush = flush;
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

void quicktime_init_codec_div5(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_DX50,
		"DIVX",
		"Mike Row Soft MPEG4 Version 5");
	result->ffmpeg_id = CODEC_ID_MPEG4;
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

void quicktime_init_codec_div3v2(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_MP42,
		"MP42",
		"Mike Row Soft MPEG4 Version 2");
	result->ffmpeg_id = CODEC_ID_MSMPEG4V2;
}

// Generic MPEG-4
void quicktime_init_codec_divx(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_DIVX,
		"MPEG-4",
		"Generic MPEG Four");
	result->ffmpeg_id = CODEC_ID_MPEG4;
	result->use_encore = 1;
}

void quicktime_init_codec_mpg4(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_MPG4,
		"MPEG-4",
		"FFMPEG (msmpeg4)");
	result->ffmpeg_id = CODEC_ID_MSMPEG4V1;
}

void quicktime_init_codec_dx50(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_DX50,
		"MPEG-4",
		"FFMPEG (mpeg4)");
	result->ffmpeg_id = CODEC_ID_MPEG4;
}

// Generic MPEG-4
void quicktime_init_codec_mp4v(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_MP4V,
		"MPEG4",
		"Generic MPEG Four");
	result->ffmpeg_id = CODEC_ID_MPEG4;
//	result->use_encore = 1;
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

void quicktime_init_codec_svq3(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_SVQ3,
		"Sorenson Version 3",
		"From the chearch of codecs of yesterday's sights");
	result->ffmpeg_id = CODEC_ID_SVQ3;
}

void quicktime_init_codec_h263(quicktime_video_map_t *vtrack)
{
    quicktime_mpeg4_codec_t *result = init_common(vtrack,
        QUICKTIME_H263,
        "H.263",
        "H.263");
    result->ffmpeg_id = CODEC_ID_H263;
}

void quicktime_init_codec_xvid(quicktime_video_map_t *vtrack)
{
    quicktime_mpeg4_codec_t *result = init_common(vtrack,
        QUICKTIME_XVID,
        "XVID",
        "FFmpeg MPEG-4");
    result->ffmpeg_id = CODEC_ID_MPEG4;
}

// field based MPEG-4
void quicktime_init_codec_hv60(quicktime_video_map_t *vtrack)
{
	quicktime_mpeg4_codec_t *result = init_common(vtrack, 
		QUICKTIME_HV60,
		"Dual MPEG-4",
		"MPEG 4 with alternating streams every other frame.  (Not standardized)");
	result->total_fields = 2;
	result->ffmpeg_id = CODEC_ID_MPEG4;
}






