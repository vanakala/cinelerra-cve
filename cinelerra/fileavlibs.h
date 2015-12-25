
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
#include "aframe.inc"
#include "file.inc"
#include "filebase.h"
#include "fileavlibs.inc"
#include "mutex.inc"
#include "selection.h"
#include "vframe.inc"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

class FileAVlibs : public FileBase
{
public:
	FileAVlibs(Asset *asset, File *file);
	~FileAVlibs();

	// Get format string for ffmpeg
	static int check_sig(Asset *asset);
	int open_file(int rd, int wr);
	void close_file();

	int get_best_colormodel(Asset *asset, int driver);
	int read_frame(VFrame *frame);
	int read_aframe(AFrame *frame);

// Callbacks of FileTOC
	int get_streamcount();
	stream_params* get_track_data(int trackindex);

private:
	int convert_cmodel(AVPicture *picture_in, PixelFormat pix_fmt_in,
		int width_in, int height_in, VFrame *frame_out);
	int fill_buffer(AVFrame *avaframe, int ibytes = 0, int bps = 0, int planar = 0);
	int fill_from_buffer(int64_t rqpos, AFrame *aframe);
	static int streamformat(AVFormatContext *context);
	void liberror(int code, const char *prefix);
	static int init_picture_from_frame(AVPicture *picture, VFrame *frame);
	static AVPixelFormat color_model_to_pix_fmt(int color_model);
	static void dump_AVFormatContext(AVFormatContext *ctx, int indent = 0);
	static void dump_AVStream(AVStream *stm, int indent = 0);
	static const char *dump_ts(int64_t ts, char *obuf = 0);
	static const char *dump_AVRational(AVRational *r, char *obuf = 0);
	static const char *dump_avfmt_flag(int flags, char *obuf);

	static struct selection_int known_formats[];
	AVFormatContext *context;
	AVFrame *avvframe;
	AVFrame *avaframe;
	struct SwsContext *sws_ctx;
	SwrContext *swr_ctx;
	VFrame *temp_frame;
	FileTOC *tocfile;

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
	int num_buffers;
	int buffer_len;
	int buffer_pos;
	int64_t buffer_pts;
	double *abuffer[MAXCHANNELS];
	int buffer_valid[MAXCHANNELS];
};

#endif
