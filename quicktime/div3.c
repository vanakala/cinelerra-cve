#include "avcodec.h"
#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"


#include <pthread.h>



typedef struct
{
	int decode_initialized;
	int encode_initialized;
    AVCodec *decoder;
	AVCodecContext *decoder_context;
 	AVCodecContext *encoder_context;
    AVFrame picture;
	char *temp_frame;
	char *work_buffer;
	int buffer_size;
	int last_frame;
	int got_key;
// ID out of avcodec.h for the codec used
	int derivative;


	int bitrate;
	int bitrate_tolerance;
	int interlaced;
	int gop_size;
	int quantizer;
	int fix_bitrate;


    AVCodec *encoder;
} quicktime_div3_codec_t;

static pthread_mutex_t encode_mutex;
static pthread_mutex_t decode_mutex;
static int mutex_initialized = 0;
static int global_initialized = 0;


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

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_div3_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->decode_initialized)
	{
		pthread_mutex_lock(&decode_mutex);
    	avcodec_close(codec->decoder_context);
		free(codec->decoder_context);
		pthread_mutex_unlock(&decode_mutex);
	}
	if(codec->encode_initialized)
	{
		pthread_mutex_lock(&encode_mutex);
    	avcodec_close(codec->encoder_context);
		free(codec->encoder_context);
		pthread_mutex_unlock(&encode_mutex);
	}
	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->work_buffer) free(codec->work_buffer);
	free(codec);
}

static int init_codec(quicktime_div3_codec_t *codec, int width_i, int height_i)
{
	if(!global_initialized)
	{
		global_initialized = 1;
  		avcodec_init();
		avcodec_register_all();
	}


	codec->decoder = avcodec_find_decoder(codec->derivative);
	if(!codec->decoder)
	{
		printf("init_codec: avcodec_find_decoder returned NULL.\n");
		return 1;
	}

	codec->decoder_context = avcodec_alloc_context();
	codec->decoder_context->width = width_i;
	codec->decoder_context->height = height_i;
	if(avcodec_open(codec->decoder_context, codec->decoder) < 0)
	{
		printf("init_codec: avcodec_open failed.\n");
	}
	return 0;
}





int quicktime_div3_is_key(unsigned char *data, long size)
{
	int result = 0;

// First 2 bits are pict type.
	result = (data[0] & 0xc0) == 0;


	return result;
}


static int decode_wrapper(quicktime_div3_codec_t *codec,
	unsigned char *data, 
	long size)
{
	int got_picture = 0;
	int result;

	if(!codec->got_key && !quicktime_div3_is_key(data, size)) return 0;

	if(quicktime_div3_is_key(data, size)) codec->got_key = 1;

	result = avcodec_decode_video(codec->decoder_context, 
		&codec->picture,
		&got_picture,
		codec->work_buffer,
		size);
#ifdef ARCH_X86
	asm("emms");
#endif
	return result;
}



static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
//printf(__FUNCTION__ " div3 1\n");
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_div3_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int result = 0;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = (int)((float)width / 16 + 0.5) * 16;
	int height_i = (int)((float)height / 16 + 0.5) * 16;
	int input_cmodel;
	int bytes;
	int i;
	unsigned char **input_rows;

	init_mutex();
	pthread_mutex_lock(&decode_mutex);


	if(!codec->decode_initialized)
	{
		init_codec(codec, width_i, height_i);

		codec->decode_initialized = 1;
	}

// Handle seeking
	if(quicktime_has_keyframes(file, track) && 
		vtrack->current_position != codec->last_frame + 1)
	{
		int frame1, frame2 = vtrack->current_position;

		frame1 = quicktime_get_keyframe_before(file,
			vtrack->current_position, 
			track);

		if(frame1 < codec->last_frame &&
			frame2 > codec->last_frame) frame1 = codec->last_frame + 1;

		while(frame1 < frame2)
		{

			quicktime_set_video_position(file, frame1, track);

			bytes = quicktime_frame_size(file, frame1, track);

			if(!codec->work_buffer || codec->buffer_size < bytes)
			{
				if(codec->work_buffer) free(codec->work_buffer);
				codec->buffer_size = bytes;
				codec->work_buffer = calloc(1, codec->buffer_size + 100);
			}


			result = !quicktime_read_data(file, 
				codec->work_buffer, 
				bytes);


			if(!result)
				result = decode_wrapper(codec,
					codec->work_buffer, 
					bytes);

			frame1++;
		}

		vtrack->current_position = frame2;
	}

	codec->last_frame = vtrack->current_position;
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	quicktime_set_video_position(file, vtrack->current_position, track);

	if(!codec->work_buffer || codec->buffer_size < bytes)
	{
		if(codec->work_buffer) free(codec->work_buffer);
		codec->buffer_size = bytes;
		codec->work_buffer = calloc(1, codec->buffer_size + 100);
	}
	result = !quicktime_read_data(file, codec->work_buffer, bytes);
	
	if(!result)
		result = decode_wrapper(codec,
			codec->work_buffer, 
			bytes);

	pthread_mutex_unlock(&decode_mutex);











	result = (result <= 0);
	switch(codec->decoder_context->pix_fmt)
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


	if(!result)
	{
		int y_out_size = codec->decoder_context->width * 
			codec->decoder_context->height;
		int u_out_size = codec->decoder_context->width * 
			codec->decoder_context->height / 
			4;
		int v_out_size = codec->decoder_context->width * 
			codec->decoder_context->height / 
			4;
		int y_in_size = codec->picture.linesize[0] * 
			codec->decoder_context->height;
		int u_in_size = codec->picture.linesize[1] * 
			codec->decoder_context->height / 
			4;
		int v_in_size = codec->picture.linesize[2] * 
			codec->decoder_context->height / 
			4;
		input_rows = 
			malloc(sizeof(unsigned char*) * 
			codec->decoder_context->height);

		for(i = 0; i < codec->decoder_context->height; i++)
			input_rows[i] = codec->picture.data[0] + 
				i * 
				codec->decoder_context->width * 
				cmodel_calculate_pixelsize(input_cmodel);

		if(!codec->temp_frame)
		{
			codec->temp_frame = malloc(y_out_size +
				u_out_size +
				v_out_size);
		}

		if(codec->picture.data[0])
		{
			for(i = 0; i < codec->decoder_context->height; i++)
			{
				memcpy(codec->temp_frame + i * codec->decoder_context->width,
					codec->picture.data[0] + i * codec->picture.linesize[0],
					codec->decoder_context->width);
			}

			for(i = 0; i < codec->decoder_context->height; i += 2)
			{
				memcpy(codec->temp_frame + 
						y_out_size + 
						i / 2 * 
						codec->decoder_context->width / 2,
					codec->picture.data[1] + 
						i / 2 * 
						codec->picture.linesize[1],
					codec->decoder_context->width / 2);

				memcpy(codec->temp_frame + 
						y_out_size + 
						u_out_size + 
						i / 2 * 
						codec->decoder_context->width / 2,
					codec->picture.data[2] + 
						i / 2 * 
						codec->picture.linesize[2],
					codec->decoder_context->width / 2);
			}
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
			codec->decoder_context->width,       /* For planar use the luma rowspan */
			width);

		free(input_rows);
	}


	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
//printf(__FUNCTION__ " 1\n");
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_div3_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int result = 0;
	int width_i = (int)((float)width / 16 + 0.5) * 16;
	int height_i = (int)((float)height / 16 + 0.5) * 16;
	AVFrame pict_tmp;
	int bytes;
	quicktime_atom_t chunk_atom;
//printf(__FUNCTION__ " 1\n");

	init_mutex();
//printf(__FUNCTION__ " 1\n");

	pthread_mutex_lock(&encode_mutex);
	if(!codec->encode_initialized)
	{
		static char *video_rc_eq="tex^qComp";
		codec->encode_initialized = 1;
		if(!global_initialized)
		{
			global_initialized = 1;
  			avcodec_init();
			avcodec_register_all();
		}

		codec->encoder = avcodec_find_encoder(codec->derivative);
		if(!codec->encoder)
		{
			printf("encode: avcodec_find_encoder returned NULL.\n");
			pthread_mutex_unlock(&encode_mutex);
			return 1;
		}

		codec->encoder_context = avcodec_alloc_context();
		codec->encoder_context->frame_rate = FRAME_RATE_BASE *
			quicktime_frame_rate(file, track);
		codec->encoder_context->width = width_i;
		codec->encoder_context->height = height_i;
		codec->encoder_context->gop_size = codec->gop_size;
		codec->encoder_context->pix_fmt = PIX_FMT_YUV420P;
		codec->encoder_context->bit_rate = codec->bitrate;
		codec->encoder_context->bit_rate_tolerance = codec->bitrate_tolerance;
		codec->encoder_context->rc_eq = video_rc_eq;
		codec->encoder_context->qmin = 2;
		codec->encoder_context->qmax = 31;
		codec->encoder_context->max_qdiff = 3;
		codec->encoder_context->qblur = 0.5;
		codec->encoder_context->qcompress = 0.5;
		codec->encoder_context->me_method = ME_FULL;

		if(!codec->fix_bitrate)
		{
			codec->encoder_context->flags |= CODEC_FLAG_QSCALE;
		}

		if(codec->interlaced)
		{
			codec->encoder_context->flags |= CODEC_FLAG_INTERLACED_DCT;
		}


		avcodec_open(codec->encoder_context, codec->encoder);

		codec->work_buffer = calloc(1, width_i * height_i * 3);
		codec->buffer_size = width_i * height_i * 3;
	}
//printf(__FUNCTION__ " 1\n");



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
//printf(__FUNCTION__ " 1\n");
		if(!codec->temp_frame)
		{
			codec->temp_frame = malloc(width_i * height_i * 3 / 2);
		}
//printf(__FUNCTION__ " 1\n");

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
//printf(__FUNCTION__ " 1\n");

		pict_tmp.data[0] = codec->temp_frame;
		pict_tmp.data[1] = codec->temp_frame + width_i * height_i;
		pict_tmp.data[2] = codec->temp_frame + width_i * height_i + width_i * height_i / 4;
		pict_tmp.linesize[0] = width_i;
		pict_tmp.linesize[1] = width_i / 2;
		pict_tmp.linesize[2] = width_i / 2;
	}


//printf("encode 1\n");
	if(codec->quantizer >= 0)
		pict_tmp.quality = codec->quantizer;
	bytes = avcodec_encode_video(codec->encoder_context, 
		codec->work_buffer, 
        codec->buffer_size, 
        &pict_tmp);
	pthread_mutex_unlock(&encode_mutex);
//printf("encode 100\n");

	quicktime_write_chunk_header(file, trak, &chunk_atom);
//printf(__FUNCTION__ " 1\n");
	result = !quicktime_write_data(file, 
		codec->work_buffer, 
		bytes);
//printf(__FUNCTION__ " 1\n");

	quicktime_write_chunk_footer(file, 
					trak,
					vtrack->current_chunk,
					&chunk_atom, 
					1);
//printf(__FUNCTION__ " 1\n");
	if(pict_tmp.key_frame)
		quicktime_insert_keyframe(file, 
			vtrack->current_position, 
			track);
	vtrack->current_chunk++;

//printf(__FUNCTION__ " 100\n");




	return result;
}



static int set_parameter(quicktime_t *file,
	int track,
	char *key,
	void *value)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_div3_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(!strcasecmp(key, "div3_bitrate"))
		codec->bitrate = *(int*)value;
	else
	if(!strcasecmp(key, "div3_bitrate_tolerance"))
		codec->bitrate_tolerance = *(int*)value;
	else
	if(!strcasecmp(key, "div3_interlaced"))
		codec->quantizer = *(int*)value;
	else
	if(!strcasecmp(key, "div3_gop_size"))
		codec->gop_size = *(int*)value;
	else
	if(!strcasecmp(key, "div3_quantizer"))
		codec->quantizer = *(int*)value;
	else
	if(!strcasecmp(key, "div3_fix_bitrate"))
		codec->fix_bitrate = *(int*)value;

	return 0;
}


void quicktime_init_codec_div3(quicktime_video_map_t *vtrack)
{
	quicktime_div3_codec_t *codec;
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_div3_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = writes_colormodel;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	codec->derivative = CODEC_ID_MSMPEG4V3;
//	codec->derivative = CODEC_ID_MPEG4;
	codec->quantizer = -1;
}






void quicktime_init_codec_div4(quicktime_video_map_t *vtrack)
{
	quicktime_div3_codec_t *codec;
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_div3_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = writes_colormodel;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	codec->derivative = CODEC_ID_MPEG4;
	codec->quantizer = -1;
}




