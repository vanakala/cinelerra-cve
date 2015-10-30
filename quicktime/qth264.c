#include "libavcodec/avcodec.h"
#include "colormodels.h"
#include "funcprotos.h"
#include <pthread.h>
#include "qtffmpeg.h"
#include "quicktime.h"
#include "workarounds.h"
#include "x264.h"

#include <string.h>

typedef struct
{
// Encoder side
	x264_t *encoder[FIELDS];
	x264_picture_t *pic[FIELDS];
	x264_param_t param;

	int encode_initialized[FIELDS];

// Temporary storage for color conversions
	char *temp_frame;
// Storage of compressed data
	unsigned char *work_buffer;
// Amount of data in work_buffer
	int buffer_size;
	int total_fields;
// Set by flush
	int flushing;

// Decoder side
	quicktime_ffmpeg_t *decoder;

} quicktime_h264_codec_t;

static pthread_mutex_t h264_lock = PTHREAD_MUTEX_INITIALIZER;


static int delete_codec(quicktime_video_map_t *vtrack)
{
	int i;
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

	for(i = 0; i < codec->total_fields; i++)
	{
		if(codec->encode_initialized[i])
		{
			pthread_mutex_lock(&h264_lock);
			if(codec->pic[i])
			{
				x264_picture_clean(codec->pic[i]);
				free(codec->pic[i]);
			}
			if(codec->encoder[i])
			{
				x264_encoder_close(codec->encoder[i]);
			}
			pthread_mutex_unlock(&h264_lock);
		}
	}

	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->work_buffer) free(codec->work_buffer);
	if(codec->decoder) quicktime_delete_ffmpeg(codec->decoder);

	free(codec);
	return 0;
}


static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	int w_2 = quicktime_quantize2(width);
// ffmpeg interprets the codec height as the presentation height
	int h_2 = quicktime_quantize2(height);
	int i;
	int result = -1;
	int is_keyframe = 0;
	int frame_number = vtrack->current_position / codec->total_fields;
	int current_field = vtrack->current_position % codec->total_fields;
	quicktime_atom_t chunk_atom;
	unsigned char header[1024];
	int header_size = 0;
	int got_pps = 0;
	int got_sps = 0;
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;

	pthread_mutex_lock(&h264_lock);

	if(!codec->encode_initialized[current_field])
	{
		codec->encode_initialized[current_field] = 1;
		codec->param.i_width = w_2;
		codec->param.i_height = h_2;
		codec->param.i_fps_num = quicktime_frame_rate_n(file, track);
		codec->param.i_fps_den = quicktime_frame_rate_d(file, track);

		x264_param_t default_params;
		x264_param_default(&default_params);
// Reset quantizer if fixed bitrate
		if(codec->param.rc.i_qp_constant)
		{
			codec->param.rc.i_qp_constant = default_params.rc.i_qp_constant;
			codec->param.rc.i_qp_min = default_params.rc.i_qp_min;
			codec->param.rc.i_qp_max = default_params.rc.i_qp_max;
		}

		if(file->cpus > 1)
		{
			codec->param.i_threads = file->cpus;
		}

		codec->encoder[current_field] = x264_encoder_open(&codec->param);
		codec->pic[current_field] = calloc(1, sizeof(x264_picture_t));
		x264_picture_init(codec->pic[current_field]);
		x264_picture_alloc(codec->pic[current_field],
			X264_CSP_I420,
			codec->param.i_width,
			codec->param.i_height);
	}

	x264_picture_t *pic = codec->pic[current_field];
	pic->i_type = X264_TYPE_AUTO;
	pic->i_qpplus1 = X264_QP_AUTO;
	pic->i_pts = frame_number;

	codec->buffer_size = 0;
	int allocation = w_2 * h_2 * 3 + 100;
	if(!codec->work_buffer)
	{
		codec->work_buffer = calloc(1, allocation);
	}

	x264_picture_t pic_out;
	x264_nal_t *nals;
	int nnal = 0;
	x264_t *h = codec->encoder[current_field];

	if(!codec->flushing)
	{
		if(!row_pointers)
		{
			bzero(pic->img.plane[0], w_2 * h_2);
			bzero(pic->img.plane[1], w_2 * h_2 / 4);
			bzero(pic->img.plane[2], w_2 * h_2 / 4);
		}
		else
		if(file->color_model == BC_YUV420P)
		{
			memcpy(pic->img.plane[0], row_pointers[0], w_2 * h_2);
			memcpy(pic->img.plane[1], row_pointers[1], w_2 * h_2 / 4);
			memcpy(pic->img.plane[2], row_pointers[2], w_2 * h_2 / 4);
		}
		else
		{
			cmodel_transfer(0,
				row_pointers,
				pic->img.plane[0],
				pic->img.plane[1],
				pic->img.plane[2],
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
				pic->img.i_stride[0]);
		}

		result = x264_encoder_encode(h, &nals, &nnal, pic, &pic_out);
		if(result < 0)
			printf("frame_num %d:  nals=%d, ret=%d\n", frame_number, nnal, result);
	}
	else
	if(x264_encoder_delayed_frames(h) > 0)
	{
		result = x264_encoder_encode(h, &nals, &nnal, 0, &pic_out);
		if(result < 0)
			printf("flushing %d:  nals=%d, ret=%d\n", frame_number, nnal, result);
	}

	for(i = 0; i < nnal; i++)
	{
		int size = nals[i].i_payload;
		if(size + codec->buffer_size > allocation)
		{
			printf("qth264.c %d: overflow size=%d allocation=%d\n",
				__LINE__, size, allocation);
			break;
		}
		unsigned char *ptr = codec->work_buffer + codec->buffer_size;
		memcpy(ptr, nals[i].p_payload, size);
		codec->buffer_size += size;
		if(avcc->data_size)
			continue;
// Snoop NAL for avc
		ptr += 4;
		size -= 4;
// Synthesize header.
// Hopefully all the parameter set NAL's are present in the first frame.
		if(!header_size)
		{
			header[header_size++] = 0x01;
			header[header_size++] = 0x4d;
			header[header_size++] = 0x40;
			header[header_size++] = 0x1f;
			header[header_size++] = 0xff;
			header[header_size++] = 0xe1;
		}

		int nal_type = (*ptr & 0x1f);
// Picture parameter or sequence parameter set
		switch(nal_type)
		{
		case 0x07:
			if(got_sps) continue;
			got_sps = 1;
			break;
		case 0x08:
			if(got_pps) continue;
			got_pps = 1;
			header[header_size++] = 0x1; // Number of sps nal's.
			break;
		default:
			continue;
		}

		header[header_size++] = size >> 8;
		header[header_size++] = size;
		memcpy(&header[header_size], ptr, size);
		header_size += size;
		if(!got_sps || !got_pps) continue;

// Write header
		quicktime_set_avcc_header(avcc, header, header_size);
	}

	pthread_mutex_unlock(&h264_lock);

	if(codec->buffer_size > 0)
	{
		if(pic_out.i_type == X264_TYPE_IDR ||
				pic_out.i_type == X264_TYPE_I)
			is_keyframe = 1;

		quicktime_write_chunk_header(file, trak, &chunk_atom);
		result = !quicktime_write_data(file,
			(char*)codec->work_buffer,
			codec->buffer_size);
		quicktime_write_chunk_footer(file, trak,
			vtrack->current_chunk, &chunk_atom, 1);

		if(is_keyframe)
		{
			quicktime_insert_keyframe(file,
				vtrack->current_position,
				track);
		}

		vtrack->current_chunk++;
	}
	return result >= 0 ? 0 : -1;
}


static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;

	if(!codec->decoder)
		codec->decoder = quicktime_new_ffmpeg(
			file->cpus,
			codec->total_fields,
			CODEC_ID_H264,
			width, height,
			stsd_table);
	if(codec->decoder)
		return quicktime_ffmpeg_decode(codec->decoder,
			file,
			row_pointers,
			track);

	return 1;
}

static void flush(quicktime_t *file, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;

	if(!avcc->data_size)
		encode(file, 0, track);
	codec->flushing = 1;
	while(!encode(file, 0, track));
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
	return (colormodel == BC_YUV420P);
}

static int set_parameter(quicktime_t *file,
		int track,
		const char *key,
		void *value)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	char *compressor = quicktime_compressor(vtrack->track);

	if(quicktime_match_32(compressor, QUICKTIME_H264) ||
		quicktime_match_32(compressor, QUICKTIME_HV64))
	{
		quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
		if(!strcasecmp(key, "h264_bitrate"))
		{
			if(quicktime_match_32(compressor, QUICKTIME_H264))
				codec->param.rc.i_bitrate = *(int*)value;
			else
				codec->param.rc.i_bitrate = *(int*)value / 2;
		}
		else
		if(!strcasecmp(key, "h264_quantizer"))
		{
			codec->param.rc.i_qp_constant =
				codec->param.rc.i_qp_min =
				codec->param.rc.i_qp_max = *(int*)value;
		}
		else
		if(!strcasecmp(key, "h264_fix_bitrate"))
			codec->param.rc.i_qp_constant = (*(int*)value) / 1000;
	}
	return 0;
}

static quicktime_h264_codec_t* init_common(quicktime_video_map_t *vtrack,
	char *compressor,
	char *title,
	char *description)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_h264_codec_t *codec;

	codec_base->priv = calloc(1, sizeof(quicktime_h264_codec_t));
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

	codec = (quicktime_h264_codec_t*)codec_base->priv;
	x264_param_default(&codec->param);
	codec->param.rc.i_rc_method = X264_RC_CQP;
	return codec;
}

void quicktime_init_codec_h264(quicktime_video_map_t *vtrack)
{
	quicktime_h264_codec_t *result = init_common(vtrack,
		QUICKTIME_H264,
		"H.264",
		"H.264");
	result->total_fields = 1;
}


// field based H.264
void quicktime_init_codec_hv64(quicktime_video_map_t *vtrack)
{
	quicktime_h264_codec_t *result = init_common(vtrack,
		QUICKTIME_HV64,
		"Dual H.264",
		"H.264 with two streams alternating every other frame.  (Not standardized)");
	result->total_fields = 2;
}
