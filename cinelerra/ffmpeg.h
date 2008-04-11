#ifndef FFMPEG_H
#define FFMPEG_H

extern "C" {
#include <avcodec.h>
}

#include "asset.h"
#include "guicast.h"

#define FFMPEG_LATENCY -9

class FFMPEG
{
 public:
	FFMPEG(Asset *asset_in);
	~FFMPEG();
	int init(char *codec_string);
	int decode(uint8_t *data, long data_size, VFrame *frame_out);

	static int convert_cmodel(AVPicture *picture_in, PixelFormat pix_fmt,
				  int width_in, int height_in, 
				  VFrame *frame_out);
	static int convert_cmodel(VFrame *frame_in, VFrame *frame_out);

	static int convert_cmodel_transfer(VFrame *frame_in,VFrame *frame_out);
	static int init_picture_from_frame(AVPicture *picture, VFrame *frame);

	static CodecID codec_id(char *codec_string);

 private:
	static PixelFormat color_model_to_pix_fmt(int color_model);
	static int pix_fmt_to_color_model(PixelFormat pix_fmt);

	int got_picture;
	Asset *asset;
	AVCodec *codec;
	AVCodecContext *context;
	AVFrame *picture;
};


#endif /* FFMPEG_H */
