#ifndef FFMPEG_H
#define FFMPEG_H

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "asset.h"
#include "guicast.h"

#define FFMPEG_LATENCY -9

#if LIBAVCODEC_VERSION_MAJOR < 54
#define AVCodecID CodecID
#endif

#if LIBAVCODEC_VERSION_MAJOR < 55
#define avcodec_alloc_context3(codec) avcodec_alloc_context()
#define avcodec_open2(context, codec, opts) avcodec_open(context, codec)
#endif

#if LIBAVCODEC_VERSION_MAJOR < 56
#define AVPixelFormat PixelFormat
#define AV_PIX_FMT_YUYV422 PIX_FMT_YUYV422
#define AV_PIX_FMT_RGB24 PIX_FMT_RGB24
#define AV_PIX_FMT_RGB32 PIX_FMT_RGB32
#define AV_PIX_FMT_BGR24 PIX_FMT_BGR24
#define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
#define AV_PIX_FMT_YUV422P PIX_FMT_YUV422P
#define AV_PIX_FMT_YUV444P PIX_FMT_YUV444P
#define AV_PIX_FMT_YUV411P PIX_FMT_YUV411P
#define AV_PIX_FMT_RGB565 PIX_FMT_RGB565
#define AV_PIX_FMT_NONE PIX_FMT_NONE
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free(pic) av_free(*(pic))
#define AV_CODEC_ID_DVVIDEO CODEC_ID_DVVIDEO
#define AV_CODEC_ID_MPEG4 CODEC_ID_MPEG4
#define AV_CODEC_ID_NONE CODEC_ID_NONE
#endif

class FFMPEG
{
 public:
	FFMPEG(Asset *asset_in);
	~FFMPEG();
	int init(char *codec_string);
	int decode(uint8_t *data, int data_size, VFrame *frame_out);
	static int convert_cmodel(AVPicture *picture_in, AVPixelFormat pix_fmt,
				  int width_in, int height_in, 
				  VFrame *frame_out);
	static int convert_cmodel(VFrame *frame_in, VFrame *frame_out);

	static int convert_cmodel_transfer(VFrame *frame_in,VFrame *frame_out);
	static int init_picture_from_frame(AVPicture *picture, VFrame *frame);

	static AVCodecID codec_id(char *codec_string);
 private:

	static AVPixelFormat color_model_to_pix_fmt(int color_model);
	static int pix_fmt_to_color_model(AVPixelFormat pix_fmt);

	int got_picture;
	Asset *asset;
	AVCodec *codec;
	AVCodecContext *context;
	AVFrame *picture;
};


#endif /* FFMPEG_H */
