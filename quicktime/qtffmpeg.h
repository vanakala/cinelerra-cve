#ifndef QTFFMPEG_H
#define QTFFMPEG_H



// This must be separate from qtprivate.h to keep everyone from
// depending on avcodec.h
// FFMPEG front end for quicktime.
// Getting ffmpeg to do all the things it needs to do is so labor
// intensive, we have a front end for the ffmpeg front end.


// This front end is bastardized to support alternating fields with
// alternating ffmpeg instances.  It drastically reduces the bitrate 
// required to store interlaced video but nothing can read it but 
// Heroine Virtual.

#include "libavcodec/avcodec.h"
#include "qtprivate.h"

#if LIBAVCODEC_VERSION_MAJOR <= 54
#define avcodec_alloc_context3(codec) avcodec_alloc_context()
#define avcodec_open2(context, codec, opts) avcodec_open(context, codec)
#endif

#if LIBAVCODEC_VERSION_MAJOR < 56
#define AV_CODEC_FLAG_4MV CODEC_FLAG_4MV
#endif

#if LIBAVCODEC_VERSION_MAJOR < 57
#define AV_CODEC_ID_SVQ1 CODEC_ID_SVQ1
#define AV_CODEC_ID_MPEG4 CODEC_ID_MPEG4
#define AV_CODEC_ID_MPEG1VIDEO CODEC_ID_MPEG1VIDEO
#define AV_CODEC_ID_MPEG2VIDEO CODEC_ID_MPEG2VIDEO
#define AV_CODEC_ID_H263P CODEC_ID_H263P
#define AV_CODEC_ID_H264 CODEC_ID_H264
#define AV_CODEC_ID_FLV1 CODEC_ID_FLV1
#define AV_CODEC_ID_MSMPEG4V3 CODEC_ID_MSMPEG4V3
#define AV_CODEC_ID_MSMPEG4V2 CODEC_ID_MSMPEG4V2
#define AV_CODEC_ID_MSMPEG4V1 CODEC_ID_MSMPEG4V1
#define AV_CODEC_ID_SVQ3 CODEC_ID_SVQ3
#define AV_CODEC_ID_H263 CODEC_ID_H263
#define AV_CODEC_ID_DNXHD CODEC_ID_DNXHD

#define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
#define AV_PIX_FMT_YUYV422 PIX_FMT_YUYV422
#define AV_PIX_FMT_YUV422P PIX_FMT_YUV422P
#define AV_PIX_FMT_YUV410P PIX_FMT_YUV410P

#endif

typedef struct
{
#define FIELDS 2
// Encoding
    AVCodec *encoder[FIELDS];
	AVCodecContext *encoder_context[FIELDS];

// Decoding
	AVCodec *decoder[FIELDS];
	AVCodecContext *decoder_context[FIELDS];
	AVFrame picture[FIELDS];

// Last frame decoded
	int64_t last_frame[FIELDS];
// Rounded dimensions
	int width_i;
	int height_i;
// Original dimensions
	int width;
	int height;
	int fields;

// Temporary storage for color conversions
	char *temp_frame;
// Storage of compressed data
	unsigned char *work_buffer;
// Allocation of work_buffer
	int buffer_size;
	int ffmpeg_id;
} quicktime_ffmpeg_t;

extern int ffmpeg_initialized;
extern pthread_mutex_t ffmpeg_lock;

quicktime_ffmpeg_t* quicktime_new_ffmpeg(
	int cpus,
	int fields,
	int ffmpeg_id,
	int w,
	int h,
// FFmpeg needs this for the header
	quicktime_stsd_table_t *stsd_table);
void quicktime_delete_ffmpeg(quicktime_ffmpeg_t *ptr);
int quicktime_ffmpeg_decode(quicktime_ffmpeg_t *ffmpeg,
	quicktime_t *file,
	unsigned char **row_pointers,
	int track);

#endif
