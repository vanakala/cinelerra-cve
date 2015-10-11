#include <string.h>

#ifdef HAVE_SWSCALER
extern "C" {
#include <libswscale/swscale.h>
}
#endif

#include "mainerror.h"
#include "bcsignals.h"
#include "filebase.h"
#include "quicktime.h"
#include "ffmpeg.h"
#include "guicast.h"


FFMPEG::FFMPEG(Asset *asset)
{
	this->asset = asset;
	codec = 0;
	context = 0;
	picture = 0;
	got_picture = 0;
}

int FFMPEG::init(char *codec_string)
{
#if LIBAVCODEC_VERSION_MAJOR < 53
	avcodec_init();
#endif
	avcodec_register_all();

	AVCodecID id = codec_id(codec_string);

	codec = avcodec_find_decoder(id);
	if (codec == NULL) {
		errorbox("FFMPEG::init no decoder for '%s'", codec_string);
		return 1;
	}

	context = avcodec_alloc_context3(codec);

	if (avcodec_open2(context, codec, NULL)) {
		errorbox("FFMPEG::init avcodec_open() failed\n");
		return 1;
	}

	picture = av_frame_alloc();
	return 0;
}

FFMPEG::~FFMPEG()
{
#if LIBAVCODEC_VERSION_MAJOR < 55
	avcodec_close(context);
	av_free(context);
#else
	avcodec_free_context(&context);
#endif
	av_frame_free(&picture);
}


AVCodecID FFMPEG::codec_id(char *codec_string)
{
#define CODEC_IS(x) (! strncmp(codec_string, x, 4))

	if (CODEC_IS(QUICKTIME_DV) ||
			CODEC_IS(QUICKTIME_DVSD))
		return AV_CODEC_ID_DVVIDEO;

	if (CODEC_IS(QUICKTIME_MP4V) ||
			CODEC_IS(QUICKTIME_DIVX))
		return AV_CODEC_ID_MPEG4;

	return AV_CODEC_ID_NONE;

#undef CODEC_IS
}

AVPixelFormat FFMPEG::color_model_to_pix_fmt(int color_model)
{
	switch (color_model) 
	{
	case BC_YUV422: 
		return AV_PIX_FMT_YUYV422;
	case BC_RGB888:
		return AV_PIX_FMT_RGB24;
	case BC_BGR8888:  // NOTE: order flipped
		return AV_PIX_FMT_RGB32;
	case BC_BGR888:
		return AV_PIX_FMT_BGR24;
	case BC_YUV420P: 
		return AV_PIX_FMT_YUV420P;
	case BC_YUV422P:
		return AV_PIX_FMT_YUV422P;
	case BC_YUV444P:
		return AV_PIX_FMT_YUV444P;
	case BC_YUV411P:
		return AV_PIX_FMT_YUV411P;
	case BC_RGB565:
		return AV_PIX_FMT_RGB565;
	};

	return AV_PIX_FMT_NONE;
}

int FFMPEG::pix_fmt_to_color_model(AVPixelFormat pix_fmt)
{
	switch (pix_fmt) 
	{
	case AV_PIX_FMT_YUYV422:
		return BC_YUV422;
	case AV_PIX_FMT_RGB24:
		return BC_RGB888;
	case AV_PIX_FMT_RGB32:
		return BC_BGR8888;
	case AV_PIX_FMT_BGR24:
		return BC_BGR888;
	case AV_PIX_FMT_YUV420P:
		return BC_YUV420P;
	case AV_PIX_FMT_YUV422P:
		return BC_YUV422P;
	case AV_PIX_FMT_YUV444P:
		return BC_YUV444P;
	case AV_PIX_FMT_YUV411P:
		return BC_YUV411P;
	case AV_PIX_FMT_RGB565:
		return BC_RGB565;
	};

	return BC_TRANSPARENCY;
}

int FFMPEG::init_picture_from_frame(AVPicture *picture, VFrame *frame)
{
	int cmodel = frame->get_color_model();
	AVPixelFormat pix_fmt = color_model_to_pix_fmt(cmodel);

	int size = avpicture_fill(picture, frame->get_data(), pix_fmt, 
			frame->get_w(), frame->get_h());

	if (size < 0)
	{
		errorbox("FFMPEG::init_picture failed");
		return 1;
	}

	return size;
}


int FFMPEG::convert_cmodel(VFrame *frame_in,  VFrame *frame_out)
{
	AVPixelFormat pix_fmt_in =
		color_model_to_pix_fmt(frame_in->get_color_model());
	AVPixelFormat pix_fmt_out =
		color_model_to_pix_fmt(frame_out->get_color_model());
#ifdef HAVE_SWSCALER
	// We need a context for swscale
	struct SwsContext *convert_ctx;
	av_log_set_level(AV_LOG_QUIET);
#endif
	// do conversion within libavcodec if possible
	if (pix_fmt_in != AV_PIX_FMT_NONE && pix_fmt_out != AV_PIX_FMT_NONE) {
		// set up a temporary pictures from frame_in and frame_out
		AVPicture picture_in, picture_out;
		init_picture_from_frame(&picture_in, frame_in);
		init_picture_from_frame(&picture_out, frame_out);
		int result;
#ifndef HAVE_SWSCALER
		result = img_convert(&picture_out,
				pix_fmt_out,
				&picture_in,
				pix_fmt_in,
				frame_in->get_w(),
				frame_in->get_h());
		if (result)
		{
			errorbox("Failed to convert image with ffmpeg");
		}
#else
		convert_ctx = sws_getContext(frame_in->get_w(), frame_in->get_h(),pix_fmt_in,
				frame_out->get_w(),frame_out->get_h(),pix_fmt_out,
				SWS_BICUBIC, NULL, NULL, NULL);

		if(convert_ctx == NULL){
			printf("FFmpeg: swscale context initialization failed");
			return 1;
		}

		result = sws_scale(convert_ctx, 
				picture_in.data, picture_in.linesize,
				0, frame_in->get_h(),
				picture_out.data, picture_out.linesize);

		sws_freeContext(convert_ctx);
		if (!result)
		{
			errorbox("Failed to convert image with ffmpeg");
		}
#endif

		return result;
	}

	// failing the fast method, use the failsafe cmodel_transfer()
	return convert_cmodel_transfer(frame_in, frame_out);
}

int FFMPEG::convert_cmodel_transfer(VFrame *frame_in, VFrame *frame_out)
{
	// WARNING: cmodel_transfer is said to be broken with BC_YUV411P
	cmodel_transfer(
		// Packed data out 
		frame_out->get_rows(),
		// Packed data in
		frame_in->get_rows(),

		// Planar data out
		frame_out->get_y(), frame_out->get_u(), frame_out->get_v(),
		// Planar data in
		frame_in->get_y(), frame_in->get_u(), frame_in->get_v(),

		// Dimensions in
		0, 0, frame_in->get_w(), frame_in->get_h(),
		// Dimensions out
		0, 0, frame_out->get_w(), frame_out->get_h(),

		// Color models
		frame_in->get_color_model(), frame_out->get_color_model(),

		// Background color
		0,

		// Rowspans (of luma for YUV)
		frame_in->get_w(), frame_out->get_w());

	return 0;
}


int FFMPEG::convert_cmodel(AVPicture *picture_in, AVPixelFormat pix_fmt_in,
		int width_in, int height_in, VFrame *frame_out)
{
	// set up a temporary picture_out from frame_out
	AVPicture picture_out;
	init_picture_from_frame(&picture_out, frame_out);
	int cmodel_out = frame_out->get_color_model();
	AVPixelFormat pix_fmt_out = color_model_to_pix_fmt(cmodel_out);

#ifdef HAVE_SWSCALER
	// We need a context for swscale
	struct SwsContext *convert_ctx;
#endif
	int result;
#ifndef HAVE_SWSCALER
	// do conversion within libavcodec if possible
	if (pix_fmt_out != AV_PIX_FMT_NONE) {
		result = img_convert(&picture_out,
				pix_fmt_out,
				picture_in,
				pix_fmt_in,
				width_in,
				height_in);
	} else
		result = 1;
	if (result)
#else
	convert_ctx = sws_getContext(width_in, height_in, pix_fmt_in,
			frame_out->get_w(),frame_out->get_h(), pix_fmt_out,
			SWS_BICUBIC, NULL, NULL, NULL);

	if(convert_ctx == NULL){
		errorbox("FFMPEG: swscale context initialization failed");
		return 1;
	}

	result = sws_scale(convert_ctx, 
			picture_in->data, picture_in->linesize,
			0, height_in,
			picture_out.data, picture_out.linesize);

	sws_freeContext(convert_ctx);
	if(!result)
#endif
	{
		errorbox("Failed to convert image with ffmpeg");
		return 1;
	}

	// make an intermediate temp frame only if necessary
	int cmodel_in = pix_fmt_to_color_model(pix_fmt_in);
	if (cmodel_in == BC_TRANSPARENCY)
	{
		if (pix_fmt_in == AV_PIX_FMT_RGB32) {
			// avoid infinite recursion if things are broken
			printf("FFMPEG::convert_cmodel pix_fmt_in broken!\n");
			return 1;
		}

		// NOTE: choose RGBA8888 as a hopefully non-lossy colormodel
		VFrame *temp_frame = new VFrame(0, width_in, height_in, 
						BC_RGBA8888);
		if (convert_cmodel(picture_in, pix_fmt_in,
				  width_in, height_in, temp_frame)) {
			delete temp_frame;
			return 1;  // recursed call will print error message
		}
		
		int result = convert_cmodel(temp_frame, frame_out);
		delete temp_frame;
		return result;
	}

	// NOTE: no scaling possible in img_convert() so none possible here
	if (frame_out->get_w() != width_in ||
			frame_out->get_h() != height_in)
	{
		printf("scaling from %dx%d to %dx%d not allowed\n",
			width_in, height_in, 
			frame_out->get_w(), frame_out->get_h());
		return 1;
	}


	// if we reach here we know that cmodel_transfer() will work
	uint8_t *yuv_in[3] = {0,0,0};
	uint8_t *row_pointers_in[height_in];
	if (cmodel_is_planar(cmodel_in))
	{
		yuv_in[0] = picture_in->data[0];
		yuv_in[1] = picture_in->data[1];
		yuv_in[2] = picture_in->data[2];
	}
	else
	{
		// set row pointers for picture_in 
		uint8_t *data = picture_in->data[0];
		int bytes_per_line = 
			cmodel_calculate_pixelsize(cmodel_in) * height_in;
		for (int i = 0; i < height_in; i++)
		{
			row_pointers_in[i] = data + i * bytes_per_line;
		}
	}

	cmodel_transfer(
		// Packed data out 
		frame_out->get_rows(),
		// Packed data in
		row_pointers_in,

		// Planar data out
		frame_out->get_y(), frame_out->get_u(), frame_out->get_v(),
		// Planar data in
		yuv_in[0], yuv_in[1], yuv_in[2],

		// Dimensions in
		0, 0, width_in, height_in,  // NOTE: dimensions are same
		// Dimensions out
		0, 0, width_in, height_in,

		// Color model in, color model out
		cmodel_in, cmodel_out,

		// Background color
		0,

		// Rowspans in, out (of luma for YUV)
		width_in, width_in);

	return 0;
}

int FFMPEG::decode(uint8_t *data, int data_size, VFrame *frame_out) 
{
	// NOTE: frame must already have data space allocated

	got_picture = 0;
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(0<<8)+0)
	int length = avcodec_decode_video(context,
				picture,
				&got_picture,
				data,
				data_size);
#else
	AVPacket pkt;
	av_init_packet( &pkt );
	pkt.data = data;
	pkt.size = data_size;
	int length = avcodec_decode_video2(context,
				picture,
				&got_picture,
				&pkt);
#endif

	if (length < 0) 
	{
		errorbox("FFmpeg failed to decode frame\n");
		return 1;
	}

	if (! got_picture)
	{
		// signal the caller there is no picture yet
		return FFMPEG_LATENCY;
	}

	int result = convert_cmodel((AVPicture *)picture, 
				context->pix_fmt,
				asset->width,
				asset->height,
				frame_out);

	return result;
}
