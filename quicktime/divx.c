// Divx for encore50


#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "workarounds.h"
#include ENCORE_INCLUDE
#include DECORE_INCLUDE

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>


typedef struct
{
#define FIELDS 2
	unsigned char *work_buffer;
	char *temp_frame;
	long buffer_size;
	int decode_initialized[FIELDS];
	int encode_initialized[FIELDS];
// For heroine 60 encoding, we want different streams for each field.
	int total_fields;
	int bitrate;
	long rc_period;          // the intended rate control averaging period
	long rc_reaction_period; // the reation period for rate control
	long rc_reaction_ratio;  // the ratio for down/up rate control
	long max_key_interval;   // the maximum interval between key frames
	int max_quantizer;       // the upper limit of the quantizer
	int min_quantizer;       // the lower limit of the quantizer
	int quantizer;           // For vbr
	int quality;             // the forward search range for motion estimation
	int fix_bitrate;
	int use_deblocking;

// Last frame decoded
	long last_frame[FIELDS];
	int encode_handle[FIELDS];

	DEC_PARAM dec_param[FIELDS];
	ENC_PARAM enc_param[FIELDS];

	int decode_handle[FIELDS];
// Must count pframes in VBR
	int p_count[FIELDS];
} quicktime_divx_codec_t;

static pthread_mutex_t encode_mutex;
static pthread_mutex_t decode_mutex;
static int mutex_initialized = 0;
static int decode_handle = 1;
static int encode_handle = 0;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_divx_codec_t *codec;
	int i;
//printf("delete_codec 1\n");


	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	for(i = 0; i < codec->total_fields; i++)
	{
		if(codec->encode_initialized[i])
		{
			pthread_mutex_lock(&encode_mutex);
			encore(codec->encode_handle[i],
				ENC_OPT_RELEASE,
				0,
				0);
			pthread_mutex_unlock(&encode_mutex);
		}

		if(codec->decode_initialized[i])
		{
			pthread_mutex_lock(&decode_mutex);

			decore_set_global(&codec->dec_param[i]);
			decore(codec->decode_handle[i],
				DEC_OPT_RELEASE,
				0,
				0);

			free(codec->dec_param[i].buffers.mp4_edged_ref_buffers);
			free(codec->dec_param[i].buffers.mp4_edged_for_buffers);
			free(codec->dec_param[i].buffers.mp4_display_buffers);
			free(codec->dec_param[i].buffers.mp4_state);
			free(codec->dec_param[i].buffers.mp4_tables);
			free(codec->dec_param[i].buffers.mp4_stream);

			pthread_mutex_unlock(&decode_mutex);
		}
	}
	pthread_mutex_unlock(&decode_mutex);


	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->work_buffer) free(codec->work_buffer);


	free(codec);
//printf("delete_codec 100\n");
	return 0;
}

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








static void init_mutex()
{
	if(!mutex_initialized)
	{
		pthread_mutexattr_t attr;
		mutex_initialized = 1;
		pthread_mutexattr_init(&attr);
		pthread_mutex_init(&decode_mutex, &attr);
		pthread_mutex_init(&encode_mutex, &attr);
	}
}




// Determine of the compressed frame is a keyframe for direct copy
int quicktime_divx_is_key(unsigned char *data, long size)
{
	int result = 0;
	int i;

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
	
	return result;
}


// Test for VOL header in frame
int quicktime_divx_has_vol(unsigned char *data)
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



int quicktime_divx_write_vol(unsigned char *data_start,
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





#define READ_RAW(framenum) \
{ \
	quicktime_set_video_position(file, framenum, track); \
	bytes = quicktime_frame_size(file, framenum, track); \
	if(!codec->work_buffer || codec->buffer_size < bytes) \
	{ \
		if(codec->work_buffer) free(codec->work_buffer); \
		codec->buffer_size = bytes; \
		codec->work_buffer = calloc(1, codec->buffer_size + 100); \
	} \
	result = !quicktime_read_data(file, codec->work_buffer, bytes); \
}





static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int i;
	longest bytes;
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_divx_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = (int)((float)width / 16 + 0.5) * 16;
	int height_i = (int)((float)height / 16 + 0.5) * 16;
	DEC_FRAME dec_frame;
	int use_temp = 0;
	int input_cmodel;
	char *bmp_pointers[3];
	long temp_position;
	int current_field = vtrack->current_position % codec->total_fields;



	init_mutex();
	pthread_mutex_lock(&decode_mutex);






/*
 * printf("decode 1 %d %d %d\n", 
 * vtrack->current_position, 
 * current_field,
 * codec->decode_initialized[current_field]);
 */





	if(!codec->decode_initialized[current_field])
	{
		DEC_MEM_REQS dec_mem_reqs;


		if(!codec->temp_frame)
			codec->temp_frame = malloc(width_i * height_i * 3 / 2);

// decore requires handle to be > 1
		codec->decode_handle[current_field] = decode_handle++;
		codec->last_frame[current_field] = -1;
		codec->dec_param[current_field].x_dim = width_i;
		codec->dec_param[current_field].y_dim = height_i;
		codec->dec_param[current_field].output_format = DEC_420;
		codec->dec_param[current_field].time_incr = 0;

		decore(codec->decode_handle[current_field], 
			DEC_OPT_MEMORY_REQS, 
			&codec->dec_param[current_field], 
			&dec_mem_reqs);
		codec->dec_param[current_field].buffers.mp4_edged_ref_buffers = 
			calloc(1, dec_mem_reqs.mp4_edged_ref_buffers_size);
		codec->dec_param[current_field].buffers.mp4_edged_for_buffers = 
			calloc(1, dec_mem_reqs.mp4_edged_for_buffers_size);
		codec->dec_param[current_field].buffers.mp4_display_buffers = 
			calloc(1, dec_mem_reqs.mp4_display_buffers_size);
		codec->dec_param[current_field].buffers.mp4_state = 
			calloc(1, dec_mem_reqs.mp4_state_size);
		codec->dec_param[current_field].buffers.mp4_tables = 
			calloc(1, dec_mem_reqs.mp4_tables_size);
		codec->dec_param[current_field].buffers.mp4_stream = 
			calloc(1, dec_mem_reqs.mp4_stream_size);
//printf("decode 2\n");
		decore(codec->decode_handle[current_field], 
			DEC_OPT_INIT, 
			&codec->dec_param[current_field], 
			NULL);

//printf("decode 3\n");



// Must decode frame with VOL header first but only the first frame in the
// field sequence has a VOL header.
		temp_position = vtrack->current_position;
		READ_RAW(current_field);
		vtrack->current_position = temp_position;
		dec_frame.bitstream = codec->work_buffer;
		dec_frame.length = bytes;
		dec_frame.stride = width_i;
		dec_frame.render_flag = 0;
		dec_frame.bmp[0] = codec->temp_frame;
		dec_frame.bmp[1] = codec->temp_frame + width_i * height_i;
		dec_frame.bmp[2] = codec->temp_frame + width_i * height_i * 5 / 4;
		decore(codec->decode_handle[current_field], 0, &dec_frame, NULL);


//printf("decode 9\n");

		codec->decode_initialized[current_field] = 1;
		decore_save_global(&codec->dec_param[current_field]);
	}
//printf("decode 10\n");

	decore_set_global(&codec->dec_param[current_field]);

// Enable deblocking.  This doesn't make much difference at high bitrates.
	DEC_SET dec_set_arg;
	dec_set_arg.postproc_level = (codec->use_deblocking ? 100 : 0);
	decore(codec->decode_handle[current_field], 
		DEC_OPT_SETPP, 
		&dec_set_arg, 
		NULL);
//printf("decode 30\n");



	input_cmodel = BC_YUV420P;

// Decode directly into return values
	if(file->color_model == input_cmodel &&
		file->out_w == width_i &&
		file->out_h == height_i &&
		file->in_x == 0 &&
		file->in_y == 0 &&
		file->in_w == width_i &&
		file->in_h == height_i)
	{
//		dec_frame.dst = row_pointers[0];
		dec_frame.bmp[0] = row_pointers[0];
		dec_frame.bmp[1] = row_pointers[1];
		dec_frame.bmp[2] = row_pointers[2];
		use_temp = 0;
	}
	else
// Decode into temporaries
	{
		dec_frame.bmp[0] = codec->temp_frame;
		dec_frame.bmp[1] = codec->temp_frame + width_i * height_i;
		dec_frame.bmp[2] = codec->temp_frame + width_i * height_i * 5 / 4;
		use_temp = 1;
	}

//printf("decode 40\n");
	dec_frame.stride = width_i;












//printf("decode 1 %d %d\n", codec->last_frame, vtrack->current_position);

	if(quicktime_has_keyframes(file, track) && 
		vtrack->current_position != 
			codec->last_frame[current_field] + codec->total_fields)
	{
		int frame1, frame2 = vtrack->current_position, current_frame = frame2;

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
			frame1 = codec->last_frame[current_field] + codec->total_fields;

// Render up to current position
		while(frame1 < frame2)
		{
			READ_RAW(frame1);

			dec_frame.bitstream = codec->work_buffer;
			dec_frame.length = bytes;
			dec_frame.render_flag = 0;
			decore(codec->decode_handle[current_field], 
				0, 
				&dec_frame, 
				NULL);
			frame1 += codec->total_fields;
		}
		
		
		vtrack->current_position = frame2;
	}





//printf("decode 50\n");









	codec->last_frame[current_field] = vtrack->current_position;
//printf("decode 1\n");


	READ_RAW(vtrack->current_position);



//printf("decode 6\n");


	dec_frame.bitstream = codec->work_buffer;
	dec_frame.length = bytes;
	dec_frame.render_flag = 1;

//printf("decode 60\n");
	decore(codec->decode_handle[current_field], 0, &dec_frame, NULL);
//printf("decode 100\n");



// Now line average the Y buffer, doubling its height 
// while keeping the UV buffers the same.
/*
 * 	if(codec->total_fields == 2)
 * 	{
 * 		unsigned char *bitmap = dec_frame.bmp[0];
 * 		unsigned char *in_row1 = bitmap + width_i * height_i / 2 - width_i * 2;
 * 		unsigned char *in_row2 = in_row1 + width_i;
 * 		unsigned char *out_row1 = bitmap + width_i * height_i - width_i * 2;
 * 		unsigned char *out_row2 = out_row1 + width_i;
 * 
 * 		while(in_row1 >= bitmap)
 * 		{
 * 			unsigned char *in_ptr1 = in_row1;
 * 			unsigned char *in_ptr2 = in_row2;
 * 			unsigned char *out_ptr1 = out_row1;
 * 			unsigned char *out_ptr2 = out_row2;
 * 			if(current_field == 0)
 * 				for(i = 0; i < width_i; i++)
 * 				{
 * 					*out_ptr1++ = ((int64_t)*in_ptr1 + (int64_t)*in_ptr2) >> 1;
 * 					*out_ptr2++ = *in_ptr2;
 * 					in_ptr1++;
 * 					in_ptr2++;
 * 				}
 * 			else
 * 				for(i = 0; i < width_i; i++)
 * 				{
 * 					*out_ptr1++ = *in_ptr1;
 * 					*out_ptr2++ = ((int64_t)*in_ptr1 + (int64_t)*in_ptr2) >> 1;
 * 					in_ptr1++;
 * 					in_ptr2++;
 * 				}
 * 			in_row1 -= width_i;
 * 			in_row2 -= width_i;
 * 			out_row1 -= width_i * 2;
 * 			out_row2 -= width_i * 2;
 * 		}
 * 	}
 */

//printf("decode 110\n");

// Convert to output colorspace
	if(use_temp)
	{
		unsigned char **input_rows = malloc(sizeof(unsigned char*) * height_i);
		for(i = 0; i < height_i; i++)
			input_rows[i] = codec->temp_frame + width_i * 3;
		
		
		cmodel_transfer(row_pointers, /* Leave NULL if non existent */
			input_rows,
			row_pointers[0], /* Leave NULL if non existent */
			row_pointers[1],
			row_pointers[2],
			codec->temp_frame, /* Leave NULL if non existent */
			codec->temp_frame + width_i * height_i,
			codec->temp_frame + width_i * height_i + width_i * height_i / 4,
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
			width_i,       /* For planar use the luma rowspan */
			width);
		
		free(input_rows);
	}


	decore_save_global(&codec->dec_param[current_field]);
	pthread_mutex_unlock(&decode_mutex);


//printf("decode 120\n");

	return result;
}



static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	longest offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_divx_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = (int)((float)width / 16 + 0.5) * 16;
	int height_i = (int)((float)height / 16 + 0.5) * 16;
	int result = 0;
	int i;
	ENC_FRAME encore_input;
	ENC_RESULT encore_result;
	int current_field = vtrack->current_position % codec->total_fields;

	init_mutex();
	pthread_mutex_lock(&encode_mutex);

	if(!codec->encode_initialized[current_field])
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

// Now shrink the Y buffer by half to contain just one field
/*
 * 	if(codec->total_fields == 2)
 * 	{
 * 		unsigned char *out_ptr = encore_input.image;
 * 		unsigned char *in_ptr = encore_input.image + current_field * width_i;
 * 		int size = width_i * height_i;
 * 		while(in_ptr < (unsigned char*)encore_input.image + size)
 * 		{
 * 			for(i = 0; i < width_i; i++)
 * 				*out_ptr++ = *in_ptr++;
 * 			in_ptr += width_i;
 * 		}
 * 		bzero(out_ptr, (unsigned char*)encore_input.image + size - out_ptr);
 * 	}
 * 
 */



	if(!codec->work_buffer)
	{
		codec->buffer_size = width * height;
		codec->work_buffer = malloc(codec->buffer_size);
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
	pthread_mutex_unlock(&encode_mutex);

	result = !quicktime_write_data(file, 
		codec->work_buffer, 
		encore_input.length);
	quicktime_update_tables(file,
						trak,
						offset,
						vtrack->current_chunk,
						vtrack->current_position,
						1,
						encore_input.length);

	if(encore_result.isKeyFrame)
		quicktime_insert_keyframe(file, file->vtracks[track].current_position, track);

	file->vtracks[track].current_chunk++;


	return result;
}

static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_divx_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

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
	return 0;
}


void quicktime_init_codec_divx(quicktime_video_map_t *vtrack)
{
	quicktime_divx_codec_t *codec;

/* Init public items */
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_divx_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = writes_colormodel;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;
	
	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	
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
}

void quicktime_init_codec_hv60(quicktime_video_map_t *vtrack)
{
	quicktime_init_codec_divx(vtrack);
	quicktime_divx_codec_t *codec;
	
	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	
	codec->total_fields = 2;
}

