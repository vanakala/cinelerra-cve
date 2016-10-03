
/*
 * CINELERRA
 * Copyright (C) 2015 Einar Rünkaru <einarrunkaru@gmail dot com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef FILEAVLIBS_H
#define FILEAVLIBS_H


#include "asset.inc"
#include "avlibsconfig.h"
#include "aframe.inc"
#include "cinelerra.h"
#include "file.inc"
#include "filebase.h"
#include "fileavlibs.inc"
#include "mutex.inc"
#include "paramlist.inc"
#include "vframe.inc"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

struct avlib_formattable
{
	int fileformat;
	const char *decoder;
	const char *encoder;
};

struct avlib_encparams
{
	const char *name;
	const char *prompt;
	int type;
};

class FileAVlibs : public FileBase
{
public:
	FileAVlibs(Asset *asset, File *file);
	~FileAVlibs();

	friend class AVlibsConfig;

	// Get format string for ffmpeg
	static int check_sig(Asset *asset);
	int open_file(int rd, int wr);
	void close_file();

	int get_best_colormodel(Asset *asset, int driver);
	int read_frame(VFrame *frame);
	int read_aframe(AFrame *frame);
	int write_frames(VFrame ***frames, int len);
	static void versionifo(int indent);
	int write_aframes(AFrame **frames);
	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset, BC_WindowBase* &format_window, int options);
	static void get_render_defaults(Asset *asset);
	static const char *encoder_formatname(int fileformat);

// Callbacks of FileTOC
	int get_streamcount();
	stream_params* get_track_data(int trackindex);
	static const char *enc_prompt(const char *enc_name);
	static struct avlib_encparams encoder_params[];

private:
	int convert_cmodel(AVPicture *picture_in, PixelFormat pix_fmt_in,
		int width_in, int height_in, VFrame *frame_out);
	int convert_cmodel(VFrame *frame_in, AVPixelFormat pix_fmt_out,
		int width_out, int height_out, AVFrame *frame_out);
	int inter_color_model(int color_model);
	int decode_samples(int64_t rqpos, int length);
	int fill_buffer(AVFrame *avaframe, int ibytes = 0, int bps = 0, int planar = 0);
	int write_samples(int resampled_length, AVCodecContext *audio_ctx, ptstime pts = -1);
	AVDictionary *create_dictionary(int options);
	void list2dictionary(AVDictionary **dict, Paramlist *params);
	static int streamformat(AVFormatContext *context);
	static void liberror(int code, const char *prefix);
	static int init_picture_from_frame(AVPicture *picture, VFrame *frame);
	static AVPixelFormat color_model_to_pix_fmt(int color_model);
	static Paramlist *scan_global_options(int options);
	static Paramlist *scan_format_options(int format,
		int options, AVOutputFormat **ofmtp);
	static Paramlist *scan_options(const AVClass *avclass, int options,
		const char *listname);
	static Paramlist *scan_codecs(AVOutputFormat *oformat, Asset *asset, int options);
	static Paramlist *scan_encoder_opts(AVCodecID codec, int options);
	static Paramlist *scan_encoder_private_opts(AVCodecID codec, int options);
	static Paramlist *clean_list(Paramlist *list);
	static Param *opt2param(Paramlist *list, const AVOption *opt);
	static int skip_avoption(const AVOption *opt, int typefl);
	static void dump_AVFormatContext(AVFormatContext *ctx, int indent = 0);
	static void dump_AVStream(AVStream *stm, int indent = 0);
	static void dump_AVCodecContext(AVCodecContext *ctx, int indent = 0);
	static void dump_AVFrame(AVFrame *avf, int indent = 0);
	static void dump_AVPacket(AVPacket *pkt, int indent = 0);
	static const char *dump_ts(int64_t ts, char *obuf = 0);
	static const char *dump_AVRational(const AVRational *r, char *obuf = 0);
	static const char *dump_avfmt_flag(int flags, char *obuf);
	static const char *dump_fourcc(unsigned int tag);
	static void dump_AVCodecDescriptor(const AVCodecDescriptor *avdsc, int indent = 0);
	static void dump_AVOutputFormat(const AVOutputFormat *ofmt, int indent = 0);
	static void dump_AVOption(const AVOption *opt,
		const AVClass *avclass = 0, int indent = 0);
	static void dump_AVOptions(const AVClass *avclass, int indent = 0);
	static const char *dump_AVOptionType(enum AVOptionType type);
	static char *dump_AVOptionFlags(int flags);
	static void dump_AVCodec(const AVCodec *codec, int indent = 0);
	static const char *dump_AVMediaType(enum AVMediaType type);

	static struct avlib_formattable known_formats[];
	AVFormatContext *context;
	AVFrame *avvframe;
	AVFrame *avaframe;
	struct SwsContext *sws_ctx;
	SwrContext *swr_ctx;
	VFrame *temp_frame;
	FileTOC *tocfile;

	int reading;
	int writing;
	int headers_written;
	ptstime pts_base;
	// Streams to decode
	int audio_index;
	int video_index;
	// Next read positions
	int64_t video_pos;
	int64_t audio_pos;
	int64_t audio_delay;
	// Last decoded positions
	static Mutex *avlibs_lock;
	stream_params track_data;
	int input_channels;
	int num_buffers;
	int buffer_len;
	int buffer_pos;
	int64_t buffer_start;
	int64_t buffer_end;
	int audio_eof;
	double *abuffer[MAXCHANNELS];
	uint8_t *resampled_data[MAXCHANNELS];
	int resampled_alloc;
	static const char *ignored[];
	static const uint64_t input_layouts[];
};

#endif
