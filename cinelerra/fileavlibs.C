// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2015 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "clip.h"
#include "colormodels.h"
#include "colormodels_private.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "fileavlibs.h"
#include "filetoc.h"
#include "interlacemodes.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "mwindow.h"
#include "paramlist.h"
#include "paramlistwindow.h"
#include "pipe.h"
#include "pipeconfig.h"
#include "preferences.h"
#include "selection.h"
#include "theme.h"
#include "tmpframecache.h"
#include "videodevice.inc"
#include "vframe.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>


extern const char *version_name;

#define AVIO_BUFFSIZE 4096

struct  avlib_formattable FileAVlibs::known_formats[] =
{
	{ FILE_MOV, "mov,mp4,m4a,3gp,3g2,mj2", "mov", SUPPORTS_AUDIO | SUPPORTS_VIDEO},
	{ FILE_AVI, "avi", "avi", SUPPORTS_AUDIO | SUPPORTS_VIDEO  },
	{ FILE_AC3, "ac3", "ac3", SUPPORTS_AUDIO },
	{ FILE_OGG, "ogg", "ogg", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_MP3, "mp3", "mp3", SUPPORTS_AUDIO },
	{ FILE_MPEGTS, "mpegts", "mpegts", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_MPEG, "mpeg", "vob", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_EXR, "exr_pipe", "exr_pipe", SUPPORTS_VIDEO },
	{ FILE_TIFF, "tiff_pipe", "tiff", SUPPORTS_VIDEO },
	{ FILE_PNG, "png_pipe", "png_pipe", SUPPORTS_VIDEO },
	{ FILE_YUV, "yuv4mpegpipe", "yuv4mpegpipe",  SUPPORTS_VIDEO },
	{ FILE_RAWDV, "dv", "dv", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_MXF, "mxf", "mxf", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_MKV, "matroska,webm", "matroska", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_AU, "au", "au", SUPPORTS_AUDIO },
	{ FILE_PCM, "s16le", "s16le", SUPPORTS_AUDIO },
	{ FILE_WAV, "wav", "wav", SUPPORTS_AUDIO },
	{ FILE_FLAC, "flac", "flac", SUPPORTS_AUDIO },
	{ FILE_AIFF, "aiff", "aiff", SUPPORTS_AUDIO },
	{ FILE_3GP, 0, "3gp", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_MP4, 0, "mp4", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_PSP, 0, "psp", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_3GPP2, 0, "3g2", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_IPOD, 0, "ipod", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_ISMV, 0, "ismv", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_F4V, 0, "f4v", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_WEBM, 0, "webm", SUPPORTS_AUDIO | SUPPORTS_VIDEO },
	{ FILE_SVG, "svg_pipe", 0, SUPPORTS_VIDEO },
	{ FILE_IMAGE, "image2", 0, SUPPORTS_VIDEO },
// Needed for proper initalization of streams
	{ FILE_JPEG, "mjpeg", 0, SUPPORTS_VIDEO },
	{ FILE_TGA, "targa", 0, SUPPORTS_VIDEO },
	{ 0 }
};

const char *FileAVlibs::ignored[] =
{
	"avioflags",
	"packetsize",
	"fdebug",
	"start_time_realtime",
	"flush_packets",
	"output_ts_offset",
	"avoid_negative_ts",
	"dump_separator",
	"rtpflags",
	"ar",
	"ac",
	"debug",
	"ticks_per_frame",
	"seek2any",
	"use_wallclock_as_timestamps",
	"correct_ts_overflow",
	NULL
};

struct avlib_encparams FileAVlibs::encoder_params[] =
{
	{ "pix_fmt", N_("Pixel format:"), PARAMTYPE_INT },
	{ "framerate", N_("Framerate:"), PARAMTYPE_DBL },
	{ "samplerate", N_("Samplerate:"), PARAMTYPE_INT },
	{ "sampleformat", N_("Sample format:"), PARAMTYPE_INT },
	{ "ch_layout", N_("Channel layout:"), PARAMTYPE_LNG },
	{ 0, 0, 0 }
};

const uint64_t FileAVlibs::internal_layouts[] =
{
    0,
    AV_CH_LAYOUT_MONO,
    AV_CH_LAYOUT_STEREO,
    AV_CH_LAYOUT_2_1,
    AV_CH_LAYOUT_QUAD,
    AV_CH_LAYOUT_5POINT0,
    AV_CH_LAYOUT_6POINT0,
    AV_CH_LAYOUT_7POINT0,
    AV_CH_LAYOUT_OCTAGONAL
};

#define NUM_INTERNAL_LAYOUTS (sizeof(internal_layouts) / sizeof(uint64_t))

extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
int write_io_packet(void *opaque, uint8_t *buf, int buf_size);
}

int write_io_packet(void *opaque, uint8_t *buf, int buf_size)
{
	int fd = *(int*)opaque;

	if(write(fd, buf, buf_size) != buf_size)
		return AVERROR(EIO);

	return buf_size;
}

Mutex* FileAVlibs::avlibs_lock = new Mutex("FileAVlibs::avlibs_lock");

FileAVlibs::FileAVlibs(Asset *asset, File *file)
 : FileBase(asset, file)
{
	avvframe = 0;
	avaframe = 0;
	context = 0;
	sws_ctx = 0;
	swr_ctx = 0;
	avapkt = 0;
	avvpkt = 0;
	stream_pipe = 0;
	if(mwindow_global && edlsession->show_avlibsmsgs)
		av_log_set_level(AV_LOG_INFO);
	else
		av_log_set_level(AV_LOG_QUIET);
	num_buffers = 0;
	buffer_len = 0;
	memset(abuffer, 0, sizeof(abuffer));
	resampled_alloc = 0;
	resample_fill = 0;
	resample_size = 0;
	sample_bytes = 0;
	avio_ctx = 0;
	avio_buffer = 0;
	avio_fd = -1;
	fresh_open = 0;
	video_index = -1;
	audio_index = -1;
	current_stream = -1;
	memset(resampled_data, 0, sizeof(resampled_data));
	memset(codec_contexts, 0, sizeof(codec_contexts));
}

FileAVlibs::~FileAVlibs()
{
	for(int i = 0; i < MAXCHANNELS; i++)
		delete [] abuffer[i];
	for(int i = 0; i < MAXCHANNELS; i++)
		delete [] resampled_data[i];
	close_file();
}

// Returns: -1 - cannot open
//           0 - not avlibs file
//           1 - avlibs file
int FileAVlibs::probe_input(Asset *asset)
{
	struct stat stbuf;
	enum AVCodecID codec_id;
	AVCodec *codec;
	AVStream *stream;
	AVMediaType codec_type;
	AVCodecContext *decoder_ctx;
	double testfr;
	int stact;

	if(!asset->file_mtime.tv_sec || !asset->file_length)
	{
		if(stat(asset->path, &stbuf) < 0)
		{
			errormsg(_("Failed to find input file %s"),
				asset->path);
			return -1;
		}
		if(!S_ISREG(stbuf.st_mode))
		{
			errormsg(_("Input file '%s' is not a regular file"),
				asset->path);
			return -1;
		}
		asset->file_length = stbuf.st_size;
		asset->file_mtime = stbuf.st_mtim;
	}

	if(asset->format != FILE_UNKNOWN && asset->nb_streams)
		return 0;

	avlibs_lock->lock("FileAVlibs::probe_input");

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,10,100)
	avcodec_register_all();
#endif

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
	av_register_all();
#endif

	AVInputFormat *infmt = 0;
	AVDictionary *pcm_opts = 0;

	if(avformat_open_input(&context, asset->path, infmt, &pcm_opts) == 0)
	{
		if(avformat_find_stream_info(context, NULL) < 0)
		{
			avformat_close_input(&context);
			avlibs_lock->unlock();
			return 0;
		}

		asset->format = streamformat(context);

		if(asset->format != FILE_SVG)
			get_decoder_format_defaults(asset, context);

		for(int i = 0; i < context->nb_streams; i++)
		{
			stact = -1;
			stream = context->streams[i];
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
			decoder_ctx = stream->codec;
			codec_id = decoder_ctx->codec_id;
#else
			codec_id = stream->codecpar->codec_id;
#endif
			codec = avcodec_find_decoder(codec_id);

			if(codec_id == AV_CODEC_ID_NONE || !codec)
				continue;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
			codec_type = decoder_ctx->codec_type;
#else
			codec_type = stream->codecpar->codec_type;
			if(!(decoder_ctx = avcodec_alloc_context3(codec)))
				continue;
#endif
			codec_contexts[i] = decoder_ctx;

			switch(codec_type)
			{
			case AVMEDIA_TYPE_AUDIO:
				asset->streams[asset->nb_streams].stream_index = i;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
				asset->streams[asset->nb_streams].channels = decoder_ctx->channels;
				asset->streams[asset->nb_streams].sample_rate = decoder_ctx->sample_rate;
#else
				asset->streams[asset->nb_streams].channels = stream->codecpar->channels;
				asset->streams[asset->nb_streams].sample_rate = stream->codecpar->sample_rate;
#endif
				strncpy(asset->streams[asset->nb_streams].codec, codec->name, MAX_LEN_CODECNAME);
				asset->streams[asset->nb_streams].codec[MAX_LEN_CODECNAME - 1] = 0;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
				asset->streams[asset->nb_streams].bits = av_get_bytes_per_sample(decoder_ctx->sample_fmt) * 8;
				strncpy(asset->streams[asset->nb_streams].samplefmt,
					av_get_sample_fmt_name(decoder_ctx->sample_fmt), MAX_LEN_CODECNAME - 1);
				av_get_channel_layout_string(asset->streams[asset->nb_streams].layout, MAX_LEN_CODECNAME,
					decoder_ctx->channels, decoder_ctx->channel_layout);
#else
				asset->streams[asset->nb_streams].bits = av_get_bytes_per_sample((AVSampleFormat)stream->codecpar->format) * 8;
				strncpy(asset->streams[asset->nb_streams].samplefmt,
					av_get_sample_fmt_name((AVSampleFormat)stream->codecpar->format), MAX_LEN_CODECNAME - 1);
				av_get_channel_layout_string(asset->streams[asset->nb_streams].layout, MAX_LEN_CODECNAME,
					stream->codecpar->channels, stream->codecpar->channel_layout);
#endif
				asset->streams[asset->nb_streams].length =
					stream->duration * av_q2d(stream->time_base) *
					asset->streams[asset->nb_streams].sample_rate;
				asset->streams[asset->nb_streams].options = STRDSC_AUDIO;
				if(!asset->streams[asset->nb_streams].decoding_params[ASSET_DFORMAT_IX])
				{
					asset->streams[asset->nb_streams].decoding_params[ASSET_DFORMAT_IX] = get_decoder_params(codec);
					stact = asset->nb_streams;
				}
				asset->streams[asset->nb_streams].samplefmt[MAX_LEN_CODECNAME - 1] = 0;
				asset->streams[asset->nb_streams].layout[MAX_LEN_CODECNAME - 1] = 0;
				asset->nb_streams++;
				break;

			case AVMEDIA_TYPE_VIDEO:
				asset->streams[asset->nb_streams].stream_index = i;
				asset->streams[asset->nb_streams].channels = 1;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
				asset->streams[asset->nb_streams].width = decoder_ctx->width;
				asset->streams[asset->nb_streams].height = decoder_ctx->height;
				if(decoder_ctx->sample_aspect_ratio.den)
					asset->streams[asset->nb_streams].sample_aspect_ratio =
						av_q2d(decoder_ctx->sample_aspect_ratio);
				strncpy(asset->streams[asset->nb_streams].samplefmt,
					av_get_pix_fmt_name(decoder_ctx->pix_fmt), MAX_LEN_CODECNAME);
#else
				asset->streams[asset->nb_streams].width = stream->codecpar->width;
				asset->streams[asset->nb_streams].height = stream->codecpar->height;
				if(stream->codecpar->sample_aspect_ratio.den)
					asset->streams[asset->nb_streams].sample_aspect_ratio =
						av_q2d(stream->codecpar->sample_aspect_ratio);
				strncpy(asset->streams[asset->nb_streams].samplefmt,
					av_get_pix_fmt_name((AVPixelFormat)stream->codecpar->format), MAX_LEN_CODECNAME);
#endif
				asset->streams[asset->nb_streams].samplefmt[MAX_LEN_CODECNAME -1] = 0;
				AspectRatioSelection::limits(&asset->streams[asset->nb_streams].sample_aspect_ratio);

				// Guess usable framerate
				testfr = convert_framerate(stream->avg_frame_rate);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
				testfr = convert_framerate(decoder_ctx->framerate, testfr);
#endif
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
				testfr = convert_framerate(av_stream_get_r_frame_rate(stream), testfr);
#else
				testfr = convert_framerate(stream->r_frame_rate, testfr);
#endif
				asset->streams[asset->nb_streams].frame_rate = testfr;
				asset->streams[asset->nb_streams].length =
					stream->duration * av_q2d(stream->time_base) * testfr;
				strncpy(asset->streams[asset->nb_streams].codec, codec->name, MAX_LEN_CODECNAME);
					asset->streams[asset->nb_streams].codec[MAX_LEN_CODECNAME - 1] = 0;
				asset->streams[asset->nb_streams].options = STRDSC_VIDEO;
				if(!asset->streams[asset->nb_streams].decoding_params[ASSET_DFORMAT_IX] &&
					asset->format != FILE_SVG)
				{
					asset->streams[asset->nb_streams].decoding_params[ASSET_DFORMAT_IX] = get_decoder_params(codec);
					stact = asset->nb_streams;
				}
				asset->nb_streams++;
				// correct type of image
				if(asset->format == FILE_IMAGE)
				{
					if(codec_id == AV_CODEC_ID_TARGA)
						asset->format = FILE_TGA;
					else
					if(codec_id == AV_CODEC_ID_MJPEG)
						asset->format = FILE_JPEG;
					else
						asset->format = FILE_UNKNOWN;
				}
				break;
			}
			if(stact >= 0 && asset->streams[stact].decoding_params[ASSET_DFORMAT_IX] &&
				!asset->streams[stact].decoding_params[ASSET_DFDEFLT_IX])
			{
				Paramlist *pl = new Paramlist(asset->streams[stact].decoding_params[ASSET_DFORMAT_IX]->name);
				asset->streams[stact].decoding_params[ASSET_DFORMAT_IX]->store_defaults();
				pl->copy_top_level(asset->streams[stact].decoding_params[ASSET_DFORMAT_IX]);
				asset->streams[stact].decoding_params[ASSET_DFDEFLT_IX] = pl;
			}
		}
		asset->nb_programs = context->nb_programs;
		for(int i = 0; i < context->nb_programs; i++)
		{
			AVProgram *prg = context->programs[i];

			asset->programs[i].program_index = i;
			asset->programs[i].program_id = prg->id;
			asset->programs[i].start = (double)prg->start_time / AV_TIME_BASE;
			asset->programs[i].end = (double)prg->end_time / AV_TIME_BASE;
			asset->programs[i].nb_streams = prg->nb_stream_indexes;
			for(int j = 0; j < prg->nb_stream_indexes; j++)
				asset->programs[i].streams[j] = prg->stream_index[j];
		}
		if(asset->nb_programs)
			asset->set_program(0);
		av_dict_free(&pcm_opts);

		if(asset->format != FILE_UNKNOWN)
		{
			int fsup = supports(asset->format, 1);
			int rdsc = fsup & SUPPORTS_AUDIO ? STRDSC_AUDIO : 0;
			rdsc |= fsup & SUPPORTS_VIDEO ? STRDSC_VIDEO : 0;

// disable unsupported streams
			for(int i = 0; i < asset->nb_streams; i++)
			{
				asset->streams[i].options &= rdsc;
				if(asset->streams[i].options & STRDSC_AUDIO &&
						(asset->streams[i].sample_rate < MIN_SAMPLE_RATE))
					asset->streams[i].options &= ~STRDSC_AUDIO;
				if(asset->streams[i].options & STRDSC_VIDEO &&
						(asset->streams[i].width < MIN_FRAME_WIDTH ||
						asset->streams[i].height < MIN_FRAME_HEIGHT))
					asset->streams[i].options &= ~STRDSC_VIDEO;
			}

			switch(asset->format)
			{
			case FILE_SVG:
			case FILE_EXR:
			case FILE_JPEG:
			case FILE_PNG:
			case FILE_TGA:
			case FILE_TIFF:
				asset->set_single_image();
				break;
			}
			if(asset->single_image)
			{
				avformat_close_input(&context);
				avlibs_lock->unlock();
				return 1;
			}

			asset->tocfile = new FileTOC(this, preferences_global->index_directory,
				asset->path, asset->file_length, asset->file_mtime.tv_sec);
			if(asset->tocfile->init_tocfile(TOCFILE_TYPE_MUX1))
			{
				avformat_close_input(&context);
				delete asset->tocfile;
				asset->tocfile = 0;
				avlibs_lock->unlock();
				return -1;
			}

			for(int i = 0; i < asset->nb_streams; i++)
			{
				int fileix = asset->streams[i].stream_index;

				asset->streams[i].start = (ptstime)asset->tocfile->toc_streams[i].min_offset *
					av_q2d(context->streams[fileix]->time_base);
				asset->streams[i].end = (ptstime)asset->tocfile->toc_streams[i].max_offset *
					av_q2d(context->streams[fileix]->time_base);
				if(asset->streams[i].options & STRDSC_VIDEO)
				{
					if(asset->tocfile->toc_streams[i].data2)
					{
						// Use number of frames from TOC
						asset->streams[i].length =
							asset->tocfile->toc_streams[i].data2;
						// Average frame rate
						asset->streams[i].frame_rate =
							(double)asset->streams[i].length /
							(asset->streams[i].end -
								asset->streams[i].start);
					}
					else
					{
						asset->streams[i].length =
							av_rescale_q(asset->tocfile->toc_streams[i].max_index -
								asset->tocfile->toc_streams[i].min_index,
								context->streams[fileix]->time_base,
								av_inv_q(context->streams[fileix]->r_frame_rate));
					}
				}
				else
				if(asset->streams[i].options & STRDSC_AUDIO)
				{
					asset->streams[i].length = (samplenum)round((asset->tocfile->toc_streams[i].max_index -
						asset->tocfile->toc_streams[i].min_index) *
						av_q2d(context->streams[fileix]->time_base) *
						asset->streams[i].sample_rate);
				}
				asset->streams[i].toc_items = asset->tocfile->toc_streams[i].max_items;
				asset->set_base_pts(i);
			}
			for(int i = 0; i < context->nb_streams; i++)
			{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
				if(codec_contexts[i])
					avcodec_close(codec_contexts[i]);
				codec_contexts[i] = 0;
#else
				avcodec_free_context(&codec_contexts[i]);
#endif
			}
			avformat_close_input(&context);
			avlibs_lock->unlock();

			if(asset->stream_count(STRDSC_AUDIO | STRDSC_VIDEO))
				return 1;
		}
		return -1;
	}
	avlibs_lock->unlock();
	return 0;
}

double FileAVlibs::convert_framerate(AVRational frate, double dflt)
{
	double rate;

	if(frate.num > 0 && frate.den > 0)
	{
		rate = av_q2d(frate);
		if(rate > 1)
		{
			if(dflt < 1 || rate < dflt)
				return rate;
		}
	}
	return dflt;
}

AVPacket *FileAVlibs::allocate_packet()
{
	AVPacket *packet;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,12,100)
	packet = (AVPacket*)av_mallocz(sizeof(AVPacket));
	av_init_packet(packet);
#else
	packet = av_packet_alloc();
#endif
	return packet;
}

void FileAVlibs::deallocate_packet(AVPacket **packet)
{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,12,100)
	av_free_packet(*packet);
	av_freep(packet);
#else
	av_packet_free(packet);
#endif
}

int FileAVlibs::encoder_exists(AVOutputFormat *oformat, const char *encstr, int support)
{
	enum AVMediaType mtyp;
	AVCodecID codec;

	if(support & SUPPORTS_AUDIO)
		mtyp = AVMEDIA_TYPE_AUDIO;
	else if(support & SUPPORTS_VIDEO)
		mtyp = AVMEDIA_TYPE_VIDEO;
	else
		mtyp = AVMEDIA_TYPE_UNKNOWN;

	if(mtyp != AVMEDIA_TYPE_UNKNOWN)
	{
		codec = av_guess_codec(oformat, encstr, NULL, NULL, mtyp);

		if(codec == AV_CODEC_ID_NONE ||
				!avcodec_find_encoder(codec))
			return 1;
	}
	return 0;
}

int FileAVlibs::supports(int format, int decoding)
{
	int support = 0;
	int i;
	AVOutputFormat *oformat;
	AVCodec *encoder;
	const char *enc;

	for(i = 0; known_formats[i].fileformat; i++)
	{
		if(format == known_formats[i].fileformat)
		{
			support = known_formats[i].supports;
			break;
		}
	}
	if(support && !decoding)
	{
		if(!(enc = known_formats[i].encoder))
			return 0;

		avlibs_lock->lock("FileAVlibs::supports");
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,10,100)
		avcodec_register_all();
#endif
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
		av_register_all();
#endif
		if(!(oformat = av_guess_format(enc, NULL, NULL)))
		{
			avlibs_lock->unlock();
			return 0;
		}

		// Check if encoder really exists
		if(encoder_exists(oformat, enc, support & SUPPORTS_AUDIO))
				support &= ~SUPPORTS_AUDIO;
		if(encoder_exists(oformat, enc, support & SUPPORTS_VIDEO))
				support &= ~SUPPORTS_VIDEO;
		if(support)
			support |= SUPPORTS_LIBPARA;
		avlibs_lock->unlock();
	}
	return support;
}

int FileAVlibs::open_file(int open_mode, int streamix, const char *filepath)
{
	int result = 0;
	int rv;

	if(asset->format == FILE_UNKNOWN ||
			(asset->format == FILE_PCM && !asset->pcm_format))
		return 1;

	avlibs_lock->lock("FileAVlibs::open_file");
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,10,100)
	avcodec_register_all();
#endif
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
	av_register_all();
#endif
	reading = open_mode & FILE_OPEN_READ;
	writing = open_mode & FILE_OPEN_WRITE;
	audio_pos = 0;
	video_pos = 0;
	vpkt_pos = 0;
	apkt_pos = 0;
	apkt_duration = 0;
	file_eof = 0;
	headers_written = 0;
	write_pts_base = -1;

	if(reading)
	{
		AVDictionary *dict = 0;
		AVInputFormat *infmt = 0;

		list2dictionary(&dict, asset->decoder_parameters[FILEAVLIBS_DFORMAT_IX]);

		if(asset->format == FILE_PCM && asset->pcm_format)
		{
			infmt = av_find_input_format(asset->pcm_format);
			av_dict_set_int(&dict, "sample_rate", asset->streams[0].sample_rate, 0);
			av_dict_set_int(&dict, "channels", asset->streams[0].channels, 0);
		}

		if(avformat_open_input(&context, asset->path, infmt, &dict) == 0)
			result = avformat_find_stream_info(context, NULL);
		else
		{
			av_dict_free(&dict);
			avlibs_lock->unlock();
			return 1;
		}
		av_dict_free(&dict);
		if(result >= 0)
		{
			AVStream *stream;
			AVCodecContext *decoder_context;
			AVCodec *codec;
			AVMediaType codec_type;
			enum AVCodecID codec_id;
			const char *mediatype;

			result = 0;

			current_stream = asset->streams[streamix].stream_index;
			stream = context->streams[current_stream];

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
			decoder_context = stream->codec;
			codec_id = decoder_context->codec_id;
			codec_type = decoder_context->codec_type;
#else
			codec_id = stream->codecpar->codec_id;
			codec_type = stream->codecpar->codec_type;
#endif
			switch(codec_type)
			{
			case AVMEDIA_TYPE_AUDIO:
				mediatype = _("Audio");
				if(!(open_mode & FILE_OPEN_AUDIO))
					printf("%d is not an audio stream.\n", streamix);
				break;
			case AVMEDIA_TYPE_VIDEO:
				mediatype = _("Video");
				if(!(open_mode & FILE_OPEN_VIDEO))
					printf("%d is not a video stream.\n", streamix);
				break;
			}
			if(codec_id == AV_CODEC_ID_NONE || !(codec = avcodec_find_decoder(codec_id)))
			{
				if(codec_id != AV_CODEC_ID_NONE)
				{
					errormsg(_("%s decoder id %#x not found"),
						mediatype, codec_id);
					avlibs_lock->unlock();
					return 1;
				}
			}
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57,41,100)
			if(!(decoder_context = avcodec_alloc_context3(codec)))
			{
				errormsg(_("%s decoder context allocation failed"),
					mediatype);
				avlibs_lock->unlock();
				return 1;
			}
			avcodec_parameters_to_context(decoder_context, stream->codecpar);
#endif
			codec_contexts[current_stream] = decoder_context;

			if(asset->streams[streamix].options & STRDSC_AUDIO)
			{
				audio_delay = 0;
				buffer_start = buffer_end = 0;
			}
			else if(!asset->streams[streamix].options & STRDSC_VIDEO)
			{
				// should not happen
				printf("Uninitalized stream %d\n", streamix);
				return 1;
			}
			list2dictionary(&dict, asset->streams[streamix].decoding_params[FILEAVLIBS_DFORMAT_IX]);
			if((rv = avcodec_open2(decoder_context, codec, NULL)) < 0)
			{
				liberror(rv, _("Failed to open %s decoder"), mediatype);
				asset->streams[streamix].options = 0;
				current_stream = -1;
			}
			else
			{
#ifdef AVLIBS_BYTE_SEEK
				if(!(context->iformat->flags & AVFMT_NO_BYTE_SEEK))
					asset->streams[streamix].options |= STRDSC_SEEKBYTES;
#endif
			}
			av_dict_free(&dict);
			fresh_open = 1;
			avlibs_lock->unlock();
			return 0;
		}
		else
		{
			avlibs_lock->unlock();
			return 1;
		}
	}
	else if(writing)
	{
		AVOutputFormat *fmt;
		int rv;
		Paramlist *metalist;
		Param *aparam, *bparam;
		const char *dstpath;
		uint8_t *avio_buffer = 0;
		const char *realpath = asset->path;

		if(filepath)
			realpath = filepath;

		switch(asset->format)
		{
		case FILE_MOV:
		case FILE_AVI:
		case FILE_AC3:
		case FILE_OGG:
		case FILE_WAV:
		case FILE_AIFF:
		case FILE_AU:
		case FILE_MP3:
		case FILE_FLAC:
		case FILE_MPEG:
		case FILE_MPEGTS:
		case FILE_RAWDV:
		case FILE_MXF:
		case FILE_MKV:
		case FILE_3GP:
		case FILE_MP4:
		case FILE_PSP:
		case FILE_3GPP2:
		case FILE_IPOD:
		case FILE_ISMV:
		case FILE_F4V:
		case FILE_WEBM:
			break;
		case FILE_YUV:
			if(asset->use_pipe && asset->pipe[0])
			{
				stream_pipe = new Pipe(asset->pipe, realpath);
				if(stream_pipe->open_write())
				{
					errormsg(_("Failed to create pipe"));
					avlibs_lock->unlock();
					return 1;
				}
				avio_fd = stream_pipe->fd;
			}
			break;

		default:
			errormsg(_("Unsupported file type %d for encoding"), asset->format);
			avlibs_lock->unlock();
			return 1;
		}
		if(avio_fd < 0)
			dstpath = realpath;
		else
		{
			dstpath = 0;
			avio_buffer = (uint8_t*)av_malloc(AVIO_BUFFSIZE);
		}
		if((rv = avformat_alloc_output_context2(&context, NULL,
				encoder_formatname(asset->format), dstpath)) < 0)
		{
			liberror(rv, _("Failed to allocate output context"));
			avlibs_lock->unlock();
			return 1;
		}

		fmt = context->oformat;

		// Metadata
		AVDictionary *meta = 0;
		struct tm ctim, *ptm;
		time_t tst;
		char string[128];

		av_dict_set(&meta, "comment", version_name, 0);
		tst = time(0);
		if((ptm = gmtime_r(&tst, &ctim)) &&
				strftime(string, sizeof(string), "%FT%TZ", ptm))
			av_dict_set(&meta, "creation_time", string, 0);

		if(metalist = asset->encoder_parameters[FILEAVLIBS_METADT_IX])
		{
			if(aparam = metalist->find("copyright"))
				av_dict_set(&meta, "copyright",
					aparam->stringvalue, 0);
			if(aparam = metalist->find("title"))
				av_dict_set(&meta, "title",
					aparam->stringvalue, 0);
			if(aparam = metalist->find("author"))
				av_dict_set(&meta, "author",
					aparam->stringvalue, 0);
		}
		context->metadata = meta;

		if((streamix = asset->get_stream_ix(STRDSC_VIDEO)) >= 0)
		{
			AVCodec *codec;
			AVCodecContext *video_ctx;
			AVStream *stream;
			AVDictionary *dict = create_encoder_dictionary(SUPPORTS_VIDEO);

			if(asset->encoder_parameters[FILEAVLIBS_CODECS_IX] &&
				(aparam = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(AVL_PARAM_CODEC_VIDEO)))
			{
				if(!(codec = avcodec_find_encoder((AVCodecID)aparam->intvalue)))
				{
					errormsg(_("Could not find video encoder '%s'"),
						aparam->stringvalue);
					avlibs_lock->unlock();
					return 1;
				}
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
				if(!(stream = avformat_new_stream(context, codec)))
#else
				if(!(stream = avformat_new_stream(context, 0)))
#endif
				{
					errormsg(_("Could not allocate video stream for '%s'"),
						aparam->stringvalue);
					avlibs_lock->unlock();
					return 1;
				}
			}
			else
			{
				errormsg("Missing video encoder");
				avlibs_lock->unlock();
				return 1;
			}
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
			video_ctx = stream->codec;
#else
			if(!(video_ctx = avcodec_alloc_context3(codec)))
			{
				errormsg(_("Failed to allocate video encoder context"));
				avlibs_lock->unlock();
				return 1;
			}
#endif
			video_index = context->nb_streams - 1;
			video_ctx->width = asset->streams[streamix].width;
			video_ctx->height = asset->streams[streamix].height;
			video_ctx->time_base = av_d2q(1. / asset->streams[streamix].frame_rate, 10000);
			stream->time_base = video_ctx->time_base;
			stream->avg_frame_rate = av_d2q(asset->streams[streamix].frame_rate, 10000);
			if(aparam->subparams &&
					(bparam = aparam->subparams->find(encoder_params[ENC_PIX_FMTS].name)))
				video_ctx->pix_fmt = (AVPixelFormat)bparam->intvalue;
			else if(video_ctx->pix_fmt == AV_PIX_FMT_NONE)
				video_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			video_ctx->sample_aspect_ratio = av_d2q(asset->streams[streamix].sample_aspect_ratio, 40);
			stream->sample_aspect_ratio = video_ctx->sample_aspect_ratio;
			// Some formats want stream headers to be separate.
			if(context->oformat->flags & AVFMT_GLOBALHEADER)
				video_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			if(metalist && (aparam = metalist->find("strict")))
				av_dict_set(&dict, "strict", aparam->stringvalue, 0);

			if((rv = avcodec_open2(video_ctx, codec, &dict)) < 0)
			{
				av_dict_free(&dict);
				liberror(rv, _("Could not open video encoder"));
				avlibs_lock->unlock();
				return 1;
			}
			av_dict_free(&dict);

			if(!(avvframe = av_frame_alloc()))
			{
				errormsg(_("Could not create video_frame for encoding"));
				avlibs_lock->unlock();
				return 1;
			}
			avvframe->format = video_ctx->pix_fmt;
			avvframe->width = video_ctx->width;
			avvframe->height = video_ctx->height;
			avvframe->sample_aspect_ratio = video_ctx->sample_aspect_ratio;
			codec_contexts[video_index] = video_ctx;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57,41,100)
			if((rv = avcodec_parameters_from_context(stream->codecpar, video_ctx)) < 0)
				liberror(rv, _("Failed to copy parameters from video context"));
#endif
		}

		if((streamix = asset->get_stream_ix(STRDSC_AUDIO)) >= 0)
		{
			AVCodec *codec;
			AVCodecContext *audio_ctx;
			AVStream *stream;
			AVDictionary *dict = create_encoder_dictionary(SUPPORTS_AUDIO);

			if(asset->encoder_parameters[FILEAVLIBS_CODECS_IX] &&
				(aparam = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(AVL_PARAM_CODEC_AUDIO)))
			{
				if(!(codec = avcodec_find_encoder((AVCodecID)aparam->intvalue)))
				{
					errormsg(_("Could not find audio encoder '%s'"),
						aparam->stringvalue);
					avlibs_lock->unlock();
					return 1;
				}

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
				if(!(stream = avformat_new_stream(context, codec)))
#else
				if(!(stream = avformat_new_stream(context, 0)))
#endif
				{
					errormsg(_("Could not allocate audio stream for '%s'"),
						aparam->stringvalue);
					avlibs_lock->unlock();
					return 1;
				}
			}
			else
			{
				errormsg(_("Missing audio encoder"));
				avlibs_lock->unlock();
				return 1;
			}

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
			audio_ctx = stream->codec;
#else
			if(!(audio_ctx = avcodec_alloc_context3(codec)))
			{
				errormsg(_("Failed to allocate audio encoder context"));
				avlibs_lock->unlock();
				return 1;
			}
#endif
			audio_index = context->nb_streams - 1;
			audio_ctx->sample_rate = asset->streams[streamix].sample_rate;
			if(codec->supported_samplerates)
			{
				int i;
				for(i = 0; codec->supported_samplerates[i]; i++)
					if(codec->supported_samplerates[i] == audio_ctx->sample_rate)
						break;
				if(!codec->supported_samplerates[i])
				{
					errormsg(_("Samplerate %d is not supported by audio encoder"),
						audio_ctx->sample_rate);
					avlibs_lock->unlock();
					return 1;
				}
			}

			audio_ctx->channels = asset->streams[streamix].channels;
			audio_ctx->channel_layout = av_get_default_channel_layout(asset->streams[streamix].channels);
			if(codec->sample_fmts)
				audio_ctx->sample_fmt = codec->sample_fmts[0];

			if(aparam->subparams)
			{
				if(bparam = aparam->subparams->find(encoder_params[ENC_LAYOUTS].name))
				{
					audio_ctx->channel_layout = bparam->longvalue;
					audio_ctx->channels = av_get_channel_layout_nb_channels(bparam->longvalue);
				}
				if(bparam = aparam->subparams->find(encoder_params[ENC_SAMPLE_FMTS].name))
					audio_ctx->sample_fmt = (AVSampleFormat)bparam->intvalue;
			}
			stream->time_base = (AVRational){1, audio_ctx->sample_rate};
			audio_ctx->time_base = stream->time_base;

			if(metalist && (aparam = metalist->find("strict")))
				av_dict_set(&dict, "strict", aparam->stringvalue, 0);

			if(context->oformat->flags & AVFMT_GLOBALHEADER)
				audio_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			if((rv = avcodec_open2(audio_ctx, codec, &dict)) < 0)
			{
				av_dict_free(&dict);
				liberror(rv, _("Could not open audio encoder"));
				avlibs_lock->unlock();
				return 1;
			}
			av_dict_free(&dict);

			input_channels = MIN(asset->streams[streamix].channels, (NUM_INTERNAL_LAYOUTS - 1));

			if(!(swr_ctx = swr_alloc_set_opts(NULL,
				audio_ctx->channel_layout,
				audio_ctx->sample_fmt,
				audio_ctx->sample_rate,
				internal_layouts[input_channels],
				AV_SAMPLE_FMT_DBLP,
				asset->streams[streamix].sample_rate, 0, 0)))
			{
				errormsg(_("Can't allocate resample context for encoding"));
				avlibs_lock->unlock();
				return 1;
			}
			if(swr_init(swr_ctx))
			{
				errormsg(_("Failed to initalize resample context for encoding"));
				avlibs_lock->unlock();
				return 1;
			}

			if(!(avaframe = av_frame_alloc()))
			{
				errormsg(_("Could not allocate audio frame for encoding"));
				avlibs_lock->unlock();
				return 1;
			}
			avaframe->format = audio_ctx->sample_fmt;
			avaframe->channel_layout = audio_ctx->channel_layout;
			avaframe->sample_rate = audio_ctx->sample_rate;
			codec_contexts[audio_index] = audio_ctx;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57,41,100)
			if((rv = avcodec_parameters_from_context(stream->codecpar, audio_ctx)) < 0)
				liberror(rv, _("Failed to copy parameters from audio context"));
#endif
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(55,78,100)
			avaframe->nb_samples = audio_ctx->frame_size;

			if((rv = av_frame_get_buffer(avaframe, 0)) < 0)
			{
				liberror(rv, _("Failed to get audio encoding buffer"));
				avlibs_lock->unlock();
				return 1;
			}
#endif
		}

		if(video_index < 0 && audio_index < 0)
		{
			avlibs_lock->unlock();
			return 1;
		}

		if(avio_buffer)
		{
			if(!(avio_ctx = avio_alloc_context(avio_buffer, AVIO_BUFFSIZE,
					1, &avio_fd, 0, write_io_packet, 0)))
			{
				errormsg(_("Failed to allocate output IO context"));
				avlibs_lock->unlock();
				return 1;
			}
			avio_buffer = 0;
			context->pb = avio_ctx;
		}
		else
		if(!(fmt->flags & AVFMT_NOFILE))
		{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
			if((rv = avio_open(&context->pb, context->filename, AVIO_FLAG_WRITE)) < 0)
#else
			if((rv = avio_open(&context->pb, context->url, AVIO_FLAG_WRITE)) < 0)
#endif
			{
				liberror(rv, _("Could not open output file '%s'"),
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
					context->filename);
#else
					context->url);
#endif
				avlibs_lock->unlock();
				return 1;
			}
		}

		if((rv = avformat_write_header(context, 0)) < 0)
		{
			liberror(rv, _("Failed to write header"));
			avlibs_lock->unlock();
			return 1;
		}
		headers_written = 1;
	}
	avlibs_lock->unlock();
	return result;
}

void FileAVlibs::close_file()
{
	int rv;

	avlibs_lock->lock("FileAVlibs:close_file");

	if(context)
	{
		if(reading)
		{
			for(int i = 0; i < context->nb_streams; i++)
			{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
				if(codec_contexts[i])
					avcodec_close(codec_contexts[i]);
				codec_contexts[i] = 0;
#else
				avcodec_free_context(&codec_contexts[i]);
#endif
			}
			sws_freeContext(sws_ctx);
			avformat_close_input(&context);
		}
		else if(writing)
		{
			if(avaframe && headers_written)
			{
				int got_output, chan;
				AVCodecContext *audio_ctx = codec_contexts[audio_index];

				// Last resampled samples
				if(resample_fill)
				{
					avaframe->nb_samples = resample_fill;
					avaframe->pts = audio_pos;
					if(av_sample_fmt_is_planar(audio_ctx->sample_fmt))
					{
						for(chan = 0; chan < audio_ctx->channels; chan++)
							avaframe->data[chan] = resampled_data[chan];
					}
					else
						avaframe->data[0] = resampled_data[0];
				}

				// Get out samples kept in encoder
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,47,100)
				if(resample_fill)
					avcodec_send_frame(audio_ctx, avaframe);
				// Send eof to encoder
				avcodec_send_frame(audio_ctx, NULL);

				while((rv = avcodec_receive_packet(audio_ctx, avapkt)) >= 0)
				{
					avapkt->stream_index = audio_index;
					av_packet_rescale_ts(avapkt, audio_ctx->time_base,
						context->streams[audio_index]->time_base);
					if((rv = av_interleaved_write_frame(context, avapkt)) < 0)
					{
						liberror(rv, _("Failed to write last audio packet"));
						break;
					}
				}

				if(rv < 0 && rv != AVERROR_EOF)
					liberror(rv, _("Failed to encode last audio packet"));
#else
				for(got_output = 1; got_output;)
				{
					if(avcodec_encode_audio2(audio_ctx, avapkt, resample_fill ? avaframe : 0, &got_output))
					{
						errormsg(_("Failed to encode last audio packet"));
						break;
					}
					if(resample_fill)
					{
						resample_fill = 0;
						continue;
					}
					if(got_output && avapkt->size)
					{
						avapkt->stream_index = audio_index;
						av_packet_rescale_ts(avapkt, audio_ctx->time_base,
							context->streams[audio_index]->time_base);
						if((rv = av_interleaved_write_frame(context, avapkt)) < 0)
						{
							liberror(rv, _("Failed to write last audio packet"));
							break;
						}
					}
				}
#endif
			}

			if(avvframe && headers_written
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,24,102)
				&& !(context->oformat->flags & AVFMT_RAWPICTURE &&
				context->streams[video_index]->codec->codec->id == AV_CODEC_ID_RAWVIDEO)
#endif
				)
			{
				int got_output;
				AVCodecContext *video_ctx = codec_contexts[video_index];

				// Get out samples kept in encoder
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,47,100)
				avcodec_send_frame(video_ctx, NULL);

				while((rv = avcodec_receive_packet(video_ctx, avvpkt)) >= 0)
				{
					avvpkt->stream_index = video_index;
					av_packet_rescale_ts(avvpkt,
						video_ctx->time_base,
						context->streams[video_index]->time_base);

					if((rv = av_interleaved_write_frame(context, avvpkt)) < 0)
					{
						liberror(rv, _("Failed to write last video packet"));
						rv = 0;
						break;
					}
				}

				if(rv < 0 && rv != AVERROR_EOF)
					liberror(rv, _("Failed to encode last video packet"));
#else
				for(got_output = 1; got_output;)
				{
					if(avcodec_encode_video2(video_ctx, avvpkt, 0, &got_output))
					{
						errormsg(_("Failed to encode last video packet"));
						break;
					}
					if(got_output)
					{
						avvpkt->stream_index = video_index;
						av_packet_rescale_ts(avvpkt,
							video_ctx->time_base,
							context->streams[video_index]->time_base);

						if((rv = av_interleaved_write_frame(context, avvpkt)) < 0)
						{
							liberror(rv, _("Failed to write last video packet"));
							break;
						}
					}
				}
#endif
			}
			if(headers_written)
			{
				if(rv = av_write_trailer(context))
					liberror(rv, _("Failed to write trailer"));
			}
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
			if(video_index >= 0)
				avcodec_close(context->streams[video_index]->codec);
			if(audio_index >= 0)
				avcodec_close(context->streams[audio_index]->codec);
#else
			if(video_index >= 0)
				avcodec_free_context(&codec_contexts[video_index]);
			if(audio_index >= 0)
				avcodec_free_context(&codec_contexts[audio_index]);
#endif
			if(avio_ctx)
			{
				av_freep(&avio_ctx->buffer);
				av_freep(&avio_ctx);
			}
			else
			if(avio_buffer)
				av_freep(&avio_buffer);
			else
			if(!(context->oformat->flags & AVFMT_NOFILE))
				avio_closep(&context->pb);
			avformat_free_context(context);
			delete stream_pipe;
			stream_pipe = 0;
			context = 0;
		}
	}
	if(avvframe)
		av_frame_free(&avvframe);
	if(avaframe)
		av_frame_free(&avaframe);
	if(swr_ctx)
		swr_free(&swr_ctx);
	deallocate_packet(&avapkt);
	deallocate_packet(&avvpkt);
	avlibs_lock->unlock();
}

AVDictionary *FileAVlibs::create_encoder_dictionary(int options)
{
	AVDictionary *dict = 0;

	list2dictionary(&dict, asset->encoder_parameters[FILEAVLIBS_FORMAT_IX]);
	if(options & SUPPORTS_AUDIO)
	{
		list2dictionary(&dict, asset->encoder_parameters[FILEAVLIBS_ACODEC_IX]);
		list2dictionary(&dict, asset->encoder_parameters[FILEAVLIBS_APRIVT_IX]);
	}
	else if(options & SUPPORTS_VIDEO)
	{
		list2dictionary(&dict, asset->encoder_parameters[FILEAVLIBS_VCODEC_IX]);
		list2dictionary(&dict, asset->encoder_parameters[FILEAVLIBS_VPRIVT_IX]);
	}
	return dict;
}

void FileAVlibs::list2dictionary(AVDictionary **dict, Paramlist *params)
{
	Param *current;
	char buff[BCTEXTLEN];

	if(!params)
		return;

	for(current = params->first; current; current = current->next)
	{
		switch(current->type)
		{
		case PARAMTYPE_INT:
			sprintf(buff, "%d", current->intvalue);
			break;
		case PARAMTYPE_LNG:
			sprintf(buff, "%" PRId64, current->longvalue);
			break;
		case PARAMTYPE_DBL:
			sprintf(buff, "%g", current->floatvalue);
			break;
		}
		if(current->type == PARAMTYPE_STR)
			av_dict_set(dict, current->name, current->stringvalue, 0);
		else
			av_dict_set(dict, current->name, buff, 0);
	}
}

//
// video_pos end of last frame got from decoder
// vpkt_pos end of last pkt fed to decoder
//
int FileAVlibs::read_frame(VFrame *frame)
{
	int res, sres;
	int error = 0;
	int got_it;
	int64_t rqpos;

	avlibs_lock->lock("AVlibs::read_frame");
	AVStream *stream = context->streams[current_stream];
	AVCodecContext *decoder_context = codec_contexts[current_stream];

	if(!avvframe)
		avvframe = av_frame_alloc();
	if(!avvpkt)
		avvpkt = allocate_packet();

	if(!asset->single_image)
		rqpos = round((frame->get_source_pts() + asset->base_pts()) / av_q2d(stream->time_base));
	else
		rqpos = 0;

// video_pos..vpkt_pos is in decoder
	sres = 0;
	if(rqpos < video_pos || rqpos > vpkt_pos)
	{
		if((sres = media_seek(current_stream, rqpos, avvpkt, vpkt_pos)) < 0)
		{
			avlibs_lock->unlock();
			return 1;
		}
	}

	for(;;)
	{
		if(!file_eof && sres <= 0)
			error = av_read_frame(context, avvpkt);

		if(error)
		{
			if(error != AVERROR_EOF)
				break;
			file_eof = 1;
			error = 0;
		}
		sres = 0;

		if(file_eof)
		{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,24,102)
			av_free_packet(avvpkt);
#else
			av_packet_unref(avvpkt);
#endif
			avvpkt->stream_index = current_stream;
		}

		if(avvpkt->stream_index == current_stream)
		{
			if(!file_eof)
			{
				if(avvpkt->pts != AV_NOPTS_VALUE)
					vpkt_pos = avvpkt->pts + avvpkt->duration;
				else
					vpkt_pos = avvpkt->dts + avvpkt->duration;
			}
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,47,100)
			if((res = avcodec_decode_video2(decoder_context,
				avvframe, &got_it, avvpkt)) < 0)
			{
				liberror(res, _("Failed to send packet to video decoder"));
				av_packet_unref(avvpkt);
				avlibs_lock->unlock();
				return 1;
			}

			if(got_it)
			{
				video_pos = av_frame_get_best_effort_timestamp(avvframe) +
					av_frame_get_pkt_duration(avvframe);
				if(video_pos > rqpos)
					break;
			}
#else
			if(!file_eof && (res = avcodec_send_packet(decoder_context, avvpkt)) < 0)
			{
				liberror(res, _("Failed to send packet to video decoder"));
				av_packet_unref(avvpkt);
				avlibs_lock->unlock();
				return 1;
			}
			got_it = 0;
			res = avcodec_receive_frame(decoder_context, avvframe);
			if(!res)
			{
				got_it = 1;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
				video_pos = av_frame_get_best_effort_timestamp(avvframe) +
					av_frame_get_pkt_duration(avvframe);
#else
				video_pos = avvframe->best_effort_timestamp +
					avvframe->pkt_duration;
#endif
				if(video_pos > rqpos)
					break;
			}
			else
			if(res != AVERROR(EAGAIN))
			{
				liberror(res, _("Failed to decode video frame"));
				avlibs_lock->unlock();
				return 1;
			}
#endif
			if(file_eof)
				break;
		}
	}

	if(!error && got_it)
	{
		const AVPixFmtDescriptor *pix_desc = av_pix_fmt_desc_get(decoder_context->pix_fmt);

		convert_cmodel(avvframe, decoder_context->pix_fmt,
			decoder_context->width, decoder_context->height, frame);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
		frame->set_source_pts(av_frame_get_best_effort_timestamp(avvframe) *
#else
		frame->set_source_pts(avvframe->best_effort_timestamp *
#endif
			av_q2d(stream->time_base) - asset->base_pts());
		frame->set_duration(
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
			av_frame_get_pkt_duration(avvframe) *
#else
			avvframe->pkt_duration *
#endif
			av_q2d(stream->time_base));
		frame->set_frame_number(round(frame->get_source_pts() * av_q2d(stream->avg_frame_rate)));
		frame->clear_status();
		if(pix_desc->flags & AV_PIX_FMT_FLAG_ALPHA)
			frame->set_transparent();
	}
	avlibs_lock->unlock();

	if(error)
	{
		liberror(error, _("Video reading error"));
		return 1;
	}
	return 0;
}

// Variables of read_aframe and its funcions:
//   audio_pos: stream position in stream units
//   rqpos: requested position in stream_units
//   rqlen: requested length in stream_units
//   buffer_start, buffer_end in stream_units
//   buffer_pos: end of abuffer in samples of aframe

int FileAVlibs::read_aframe(AFrame *aframe)
{
	int64_t rqpos, rqlen;
	AVStream *stream = context->streams[current_stream];
	AVCodecContext *decoder_context = codec_contexts[current_stream];
	int num_ch = decoder_context->channels;
	int error;

	avlibs_lock->lock("FileAVlibs::read_aframe");

	if(!swr_ctx)
	{
		swr_ch_layout = decoder_context->channel_layout;

		if(!swr_ch_layout)
			swr_ch_layout = av_get_default_channel_layout(num_ch);

		swr_samplerate = aframe->get_samplerate();

		swr_ctx = swr_alloc_set_opts(NULL,
			swr_ch_layout,
			AV_SAMPLE_FMT_DBLP,           // out sample format
			aframe->get_samplerate(),     // out samplerate
			swr_ch_layout,
			decoder_context->sample_fmt,  // in sample fmt
			decoder_context->sample_rate, // in sample rate
			0, 0);

		if(!swr_ctx)
		{
			errormsg(_("Failed to allocate resampler context for audio decoding"));
			avlibs_lock->unlock();
			return -1;
		}
		if(error = swr_init(swr_ctx))
		{
			errormsg(_("Failed to initalize resampler context when decoding (%d)"), error);
			avlibs_lock->unlock();
			return -1;
		}
	}
	else if(swr_samplerate != aframe->get_samplerate())
	{
		swr_samplerate = aframe->get_samplerate();

		swr_ctx = swr_alloc_set_opts(swr_ctx,
			swr_ch_layout,
			AV_SAMPLE_FMT_DBLP,           // out sample format
			swr_samplerate,               // out samplerate
			swr_ch_layout,
			decoder_context->sample_fmt,  // in sample fmt
			decoder_context->sample_rate, // in sample rate
			0, 0);
		if(!swr_ctx)
		{
			errormsg(_("Failed to reconfigure resampler context for audio decoding"));
			avlibs_lock->unlock();
			return -1;
		}
		if(error = swr_init(swr_ctx))
		{
			errormsg(_("Failed to reinitalize resampler context when decoding (%d)"), error);
			avlibs_lock->unlock();
			return -1;
		}
	}

	if(buffer_len < aframe->get_buffer_length() || num_ch != num_buffers)
	{
		int j;
		for(j = 0; j < num_ch; j++)
		{
			if(abuffer[j])
				delete [] abuffer[j];
			abuffer[j] = new double[aframe->get_buffer_length()];
		}
		for(; j < MAXCHANNELS; j++)
		{
			if(!abuffer[j])
				break;
			delete [] abuffer[j];
			abuffer[j] = 0;
		}
		num_buffers = num_ch;
		buffer_len = aframe->get_buffer_length();
		buffer_start = buffer_end = 0;
		buffer_pos = 0;
	}
	rqpos = round((aframe->get_source_pts() + asset->base_pts()) / av_q2d(stream->time_base));
	rqlen = round(aframe->get_source_duration() / av_q2d(stream->time_base));

	if(rqpos != buffer_start || (rqpos + rqlen > buffer_end && !file_eof))
	{
		buffer_start = buffer_end = rqpos;
		buffer_pos = 0;

		if(error = decode_samples(rqpos, aframe->fill_length() - buffer_pos))
		{
			apkt_duration = 0;
			avlibs_lock->unlock();
			return error;
		}
		buffer_end = round((double)buffer_pos / aframe->get_samplerate() /
			av_q2d(stream->time_base)) + buffer_start;
	}

	int copylen = MIN(buffer_pos, aframe->fill_length());
	aframe->set_filled(copylen);
	memcpy(aframe->buffer, abuffer[aframe->channel],
		copylen * sizeof(double));
	fresh_open = 0;
	avlibs_lock->unlock();
	return 0;
}

int FileAVlibs::decode_samples(int64_t rqpos, int length)
{
	int error = 0;
	int got_it, sres;
	int res = 0;
	AVCodecContext *decoder_context = codec_contexts[current_stream];

	if(!avaframe)
		avaframe = av_frame_alloc();
	if(!avapkt)
		avapkt = allocate_packet();

	if(rqpos < audio_pos - apkt_duration || rqpos > apkt_pos + apkt_duration)
	{

		if((sres = media_seek(current_stream, rqpos, avapkt, apkt_pos + apkt_duration)) < 0)
			return -1;

		if(sres > 0)
		{
			swr_init(swr_ctx);
			apkt_duration = 0;
			audio_delay = 0;
		}
		while(audio_pos < rqpos && (sres || (av_read_frame(context, avapkt) == 0)))
		{
			sres = 0;

			if(avapkt->stream_index == current_stream)
			{
				apkt_duration = avapkt->duration;

				if(avapkt->pts != AV_NOPTS_VALUE)
					apkt_pos = avapkt->pts + apkt_duration;
				else
					apkt_pos = avapkt->dts + apkt_duration;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,47,100)
				if((res = avcodec_decode_audio4(decoder_context,
					avaframe, &got_it, avapkt)) < 0)
#else
				if((res = avcodec_send_packet(decoder_context, avapkt)) < 0)
				{
					if(!fresh_open)
					{
						fputs(_("Failed to send packet to audio decoder when skiping.\n"), stdout);
						return res;
					}
				}
				got_it = 0;
				res = avcodec_receive_frame(decoder_context, avaframe);
				if(!res)
					got_it = 1;
				else
				if(res != AVERROR(EAGAIN))
#endif
				{
					if(!fresh_open)
					{
						printf(_("Audio decoding failed when skiping.\n"));
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,24,102)
						av_free_packet(avapkt);
#else
						av_packet_unref(avapkt);
#endif
						return res;
					}
				}

				if(got_it)
				{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
					audio_pos = av_frame_get_best_effort_timestamp(avaframe) +
						av_frame_get_pkt_duration(avaframe);
#else
					audio_pos = avaframe->best_effort_timestamp +
						avaframe->pkt_duration;
#endif
					if(audio_pos > rqpos)
					{
						int inpos = round((audio_pos - rqpos) *
							av_q2d(context->streams[current_stream]->time_base) /
							decoder_context->sample_rate);

						res = fill_buffer(avaframe, inpos,
							av_get_bytes_per_sample(decoder_context->sample_fmt),
							av_sample_fmt_is_planar(decoder_context->sample_fmt));
						break;
					}
				}
			}
		}
	}

	if(res < 0)
		return res;

	for(;;)
	{
		if(!file_eof && (error = av_read_frame(context, avapkt)))
		{
			if(error != AVERROR_EOF)
				break;
			error = 0;
			file_eof = 1;
		}

		if(file_eof)
		{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,24,102)
			av_free_packet(avapkt);
#else
			av_packet_unref(avapkt);
#endif
			avapkt->stream_index = current_stream;
			avapkt->data = 0;
			avapkt->size = 0;
		}

		if(avapkt->stream_index == current_stream)
		{
			if(!file_eof && avapkt->pts != AV_NOPTS_VALUE)
			{
				apkt_duration = avapkt->duration;
				apkt_pos = avapkt->pts + avapkt->duration;
			}
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,47,100)
			if((res = avcodec_decode_audio4(decoder_context,
				avaframe, &got_it, avapkt)) < 0)
#else
			if((res = avcodec_send_packet(decoder_context, avapkt)) < 0)
			{
				liberror(res, _("Failed to send packet to audio decoder"));
				av_packet_unref(avapkt);
				return res;
			}
			got_it = 0;
			res = avcodec_receive_frame(decoder_context, avaframe);
			if(!res)
				got_it = 1;
			else
			if(res == AVERROR_EOF)
				file_eof = 1;
			else
			if(res != AVERROR(EAGAIN))
#endif
			{
				liberror(res, _("Audio decoding failed when reading"));
				return res;
			}

			if(got_it)
			{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
				audio_pos = av_frame_get_best_effort_timestamp(avaframe) +
					av_frame_get_pkt_duration(avaframe);
#else
				audio_pos = avaframe->best_effort_timestamp +
					avaframe->pkt_duration;
#endif
				if((res = fill_buffer(avaframe)) > 0)
				{
					if(buffer_pos >= length)
						break;
				}
				else
				{
					if(res < 0)
						error = res;
					break;
				}
			}
			if(file_eof)
				break;
		}
	}

	if(error)
	{
		liberror(error, _("Failed to read audio"));
		return error;
	}

	audio_delay = swr_get_delay(swr_ctx, avaframe->sample_rate);

	return 0;
}

int FileAVlibs::fill_buffer(AVFrame *avaframe, int insamples, int bps, int planar)
{
	int j, inpos;
	double *obp[MAXCHANNELS];
	const uint8_t *ibp[AV_NUM_DATA_POINTERS];
	int out_samples, in_length;

	for(int j = 0; j < num_buffers; j++)
		obp[j] = &abuffer[j][buffer_pos];

	inpos = insamples * bps;

	if(!planar)
	{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
		inpos *= av_frame_get_channels(avaframe);
#else
		inpos *= avaframe->channels;
#endif
	}
	for(int j = 0; j < AV_NUM_DATA_POINTERS; j++)
	{
		if(!avaframe->data[j])
			ibp[j] = 0;
		else
			ibp[j] = &avaframe->data[j][inpos];
	}
	in_length = avaframe->nb_samples - insamples;
	out_samples = swr_convert(swr_ctx,
		(uint8_t **)obp,                   // output buffer
		buffer_len - buffer_pos,           // output buffer in samples
		(const uint8_t **)&ibp,            // input samples
		in_length); // input samples

	if(out_samples > 0)
		buffer_pos += out_samples;

	return out_samples;
}

int FileAVlibs::convert_cmodel(AVFrame *picture_in, AVPixelFormat pix_fmt,
	int width_in, int height_in, VFrame *frame_out)
{
	int cmodel_out = frame_out->get_color_model();
	AVPixelFormat pix_fmt_out = color_model_to_pix_fmt(cmodel_out);
	AVPixelFormat pix_fmt_in;
	unsigned char *out_data[4];
	int out_linesizes[4];
	int result = 0;
	VFrame *temp_frame;

	if(pix_fmt_out != AV_PIX_FMT_NONE)
	{
		switch(pix_fmt)
		{
		case AV_PIX_FMT_YUVJ420P:
			pix_fmt_in = AV_PIX_FMT_YUV420P;
			break;

		case AV_PIX_FMT_YUVJ422P:
			pix_fmt_in = AV_PIX_FMT_YUV422P;
			break;

		case AV_PIX_FMT_YUVJ444P:
			pix_fmt_in = AV_PIX_FMT_YUV444P;
			break;

		case AV_PIX_FMT_YUVJ440P:
			pix_fmt_in = AV_PIX_FMT_YUV440P;
			break;

		default:
			pix_fmt_in = pix_fmt;
			break;
		}

		sws_ctx = sws_getCachedContext(sws_ctx, width_in, height_in, pix_fmt_in,
			frame_out->get_w(),frame_out->get_h(),pix_fmt_out,
			ColorModels::libinterpolate(), NULL, NULL, NULL);

		if(sws_ctx == NULL)
		{
			errormsg("Swscale context initialization failed");
			return 1;
		}

		if(pix_fmt != pix_fmt_in)
		{
			int *inv_table, *table;
			int srcRange, dstRange;
			int brightness, contrast, saturation;

			if(sws_getColorspaceDetails(sws_ctx, &inv_table,
				&srcRange, &table, &dstRange,
				&brightness, &contrast, &saturation) == 0)
			{
				sws_setColorspaceDetails(sws_ctx, inv_table,
					1, table, dstRange,
					brightness, contrast, saturation);
			}
		}

		ColorModels::fill_data(frame_out->get_color_model(), out_data, frame_out->get_data(),
			frame_out->get_y(), frame_out->get_u(), frame_out->get_v());
		ColorModels::fill_linesizes(frame_out->get_color_model(),
			frame_out->get_bytes_per_line(), frame_out->get_w(), out_linesizes);

		sws_scale(sws_ctx,
			picture_in->data, picture_in->linesize,
			0, height_in,
			out_data, out_linesizes);

		return result;
	}

// we get here if there's no direct path from the FFMPEG
// pix_fmt_in to Cinelera's frame_out colormodel.
// So-- an intermediate conversion is called for
	int temp_cmodel = ColorModels::inter_color_model(cmodel_out);

	if(cmodel_out == temp_cmodel)
	{
		// avoid infinite recursion if things are broken
		errormsg(_("pix_fmt_in broken while converting colormodel!"));
		return 1;
	}

	temp_frame = BC_Resources::tmpframes.get_tmpframe(frame_out->get_w(),
		frame_out->get_h(), temp_cmodel);

	if(convert_cmodel(picture_in, pix_fmt,
			width_in, height_in, temp_frame))
	{
		BC_Resources::tmpframes.release_frame(temp_frame);
		return 1;  // recursed call will print error message
	}

	ColorModels::copy_colors(temp_frame->get_w(), temp_frame->get_h(),
		frame_out->get_data(), frame_out->get_color_model(),
		frame_out->get_bytes_per_line(),
		temp_frame->get_data(), temp_cmodel, temp_frame->get_bytes_per_line());
	BC_Resources::tmpframes.release_frame(temp_frame);
	return 0;
}

int FileAVlibs::converts_frame()
{
	return 1;
}

int FileAVlibs::convert_cmodel(VFrame *frame_in, AVPixelFormat pix_fmt,
	int width_out, int height_out, AVFrame *frame_out)
{
	int size;
	int rv;
	AVPixelFormat pix_fmt_in = color_model_to_pix_fmt(frame_in->get_color_model());
	VFrame *temp_frame;
	AVPixelFormat pix_fmt_out;
	unsigned char *in_data[4];
	int in_linesizes[4];

	if(pix_fmt_in != AV_PIX_FMT_NONE)
	{
		ColorModels::fill_data(frame_in->get_color_model(), in_data, frame_in->get_data(),
			frame_in->get_y(), frame_in->get_u(), frame_in->get_v());
		ColorModels::fill_linesizes(frame_in->get_color_model(),
			frame_in->get_bytes_per_line(), frame_in->get_w(), in_linesizes);

		if(!frame_out->data[0])
		{
			if((rv = av_frame_get_buffer(frame_out, 32)) < 0)
			{
				liberror(rv, _("Failed to get buffer for color conversion"));
				return 1;
			}
		}

		switch(pix_fmt)
		{
		case AV_PIX_FMT_YUVJ420P:
			pix_fmt_out = AV_PIX_FMT_YUV420P;
			break;

		case AV_PIX_FMT_YUVJ422P:
			pix_fmt_out = AV_PIX_FMT_YUV422P;
			break;

		case AV_PIX_FMT_YUVJ444P:
			pix_fmt_out = AV_PIX_FMT_YUV444P;
			break;

		case AV_PIX_FMT_YUVJ440P:
			pix_fmt_out = AV_PIX_FMT_YUV440P;
			break;

		default:
			pix_fmt_out = pix_fmt;
			break;
		}

		if((sws_ctx = sws_getCachedContext(sws_ctx, frame_in->get_w(), frame_in->get_h(),
			pix_fmt_in, width_out, height_out, pix_fmt_out, ColorModels::libinterpolate(),
			0, 0, 0)) == 0)
		{
			errormsg(_("Could not get color conversion context"));
			return 1;
		}

		if(pix_fmt != pix_fmt_out)
		{
			int *inv_table, *table;
			int srcRange, dstRange;
			int brightness, contrast, saturation;

			if(sws_getColorspaceDetails(sws_ctx, &inv_table,
				&srcRange, &table, &dstRange,
				&brightness, &contrast, &saturation) == 0)
			{
				sws_setColorspaceDetails(sws_ctx, inv_table,
					srcRange, table, 1,
					brightness, contrast, saturation);
			}
		}

		if((rv = sws_scale(sws_ctx, in_data, in_linesizes,
			0, frame_in->get_h(), frame_out->data, frame_out->linesize)) < 0)
		{
			liberror(rv, _("Color conversion failed"));
			return 1;
		}
	}
	else
	{
		int cmodel = ColorModels::inter_color_model(frame_in->get_color_model());

		temp_frame = BC_Resources::tmpframes.get_tmpframe(frame_in->get_w(), frame_in->get_h(), cmodel);

		ColorModels::copy_colors(frame_in->get_w(), frame_in->get_h(),
			temp_frame->get_data(), cmodel, temp_frame->get_bytes_per_line(),
			frame_in->get_data(), frame_in->get_color_model(),
			frame_in->get_bytes_per_line());

		rv = convert_cmodel(temp_frame, pix_fmt,
			width_out, height_out, frame_out);
		BC_Resources::tmpframes.release_frame(temp_frame);
		return rv;
	}
	return 0;
}

int FileAVlibs::streamformat(AVFormatContext *context)
{
	for(int i = 0; known_formats[i].fileformat; i++)
	{
		if(known_formats[i].decoder &&
				strcmp(context->iformat->name, known_formats[i].decoder) == 0)
			return known_formats[i].fileformat;
	}
	return FILE_UNKNOWN;
}

const char *FileAVlibs::encoder_formatname(int fileformat)
{
	for(int i = 0; known_formats[i].fileformat; i++)
	{
		if(known_formats[i].fileformat == fileformat)
			return known_formats[i].encoder;
	}
	return 0;
}

void FileAVlibs::liberror(int code, const char *fmt, ...)
{
	int len;
	va_list ap;
	char string[BCTEXTLEN];

	va_start(ap, fmt);
	len = vsnprintf(string, BCTEXTLEN, fmt, ap);
	va_end(ap);
	string[len++] = ':';
	string[len++] = ' ';
	av_strerror(code, &string[len], BCTEXTLEN - 1 - len);
	errormsg("%s", string);
}

int FileAVlibs::write_frames(VFrame **frames, int len)
{
	AVCodecContext *video_ctx;
	AVStream *stream;
	int got_it, rv;

	if(video_index < 0)
		return 1;

	avlibs_lock->lock("FileAVlibs::write_frames");
	stream = context->streams[video_index];
	video_ctx = codec_contexts[video_index];

	for(int j = 0; j < len; j++)
	{
		VFrame *frame = frames[j];

		if(write_pts_base < 0)
			write_pts_base = frame->get_pts();

		if(convert_cmodel(frame, video_ctx->pix_fmt,
				video_ctx->width, video_ctx->height, avvframe))
		{
			avlibs_lock->unlock();
			return 1;
		}
		avvframe->pts = round((frame->get_pts() - write_pts_base) / av_q2d(video_ctx->time_base));

		if(asset->interlace_mode == BC_ILACE_MODE_TOP_FIRST)
		{
			avvframe->interlaced_frame = 1;
			avvframe->top_field_first = 1;
		}
		else if(asset->interlace_mode == BC_ILACE_MODE_BOTTOM_FIRST)
		{
			avvframe->interlaced_frame = 1;
			avvframe->top_field_first = 0;
		}
		else
			avvframe->interlaced_frame = 0;

		if(!avvpkt)
			avvpkt = allocate_packet();

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
		if(context->oformat->flags & AVFMT_RAWPICTURE &&
				video_ctx->codec->id == AV_CODEC_ID_RAWVIDEO)
		{
			// Avoid copy like in ffmpeg 2.8
			if(avvframe->interlaced_frame)
				video_ctx->field_order = avvframe->top_field_first ?
					AV_FIELD_TB : AV_FIELD_BT;
			else
				video_ctx->field_order = AV_FIELD_PROGRESSIVE;
			avvpkt->data = (uint8_t *)avvframe;
			avvpkt->size = sizeof(AVPicture);
			avvpkt->pts = av_rescale_q(avvframe->pts,
				video_ctx->time_base, stream->time_base);
			avvpkt->flags |= AV_PKT_FLAG_KEY;
			got_it = 1;
		}
		else
		{
			if((rv = avcodec_encode_video2(video_ctx, avvpkt, avvframe, &got_it)) < 0)
			{
				liberror(rv, _("Failed to encode video frame"));
				avlibs_lock->unlock();
				return 1;
			}
		}
		if(got_it)
		{
			avvpkt->stream_index = stream->index;
			av_packet_rescale_ts(avvpkt, video_ctx->time_base, stream->time_base);

			if((rv = av_interleaved_write_frame(context, avvpkt)) < 0)
			{
				liberror(rv, _("Failed to write video packet"));
				avlibs_lock->unlock();
				return 1;
			}
			av_packet_unref(avvpkt);
		}
#else
		if(rv = avcodec_send_frame(video_ctx, avvframe))
		{
			liberror(rv, _("Failed to send video frame to encoder"));
			avlibs_lock->unlock();
			return 1;
		}

		while((rv = avcodec_receive_packet(video_ctx, avvpkt)) >= 0)
		{
			avvpkt->stream_index = stream->index;
			av_packet_rescale_ts(avvpkt, video_ctx->time_base, stream->time_base);

			if((rv = av_interleaved_write_frame(context, avvpkt)) < 0)
			{
				liberror(rv, _("Failed to write video packet"));
				avlibs_lock->unlock();
				return 1;
			}
		}
		if(rv != AVERROR(EAGAIN))
		{
			liberror(rv, _("Failed to encode video packet"));
			avlibs_lock->unlock();
			return 1;
		}

		if((rv = av_frame_make_writable(avvframe)) < 0)
			liberror(rv, _("Can't make video frame writable"));
#endif
	}
	avlibs_lock->unlock();
	return 0;
}

int FileAVlibs::write_aframes(AFrame **frames)
{
	AVCodecContext *audio_ctx;
	AVStream *stream;
	int got_it, rv;
	int chan;
	int in_length = frames[0]->get_length();
	double *in_data[MAXCHANNELS];
	uint8_t *resampled_ptr[MAXCHANNELS];
	int resampled_length;
	int streamix = asset->get_stream_ix(STRDSC_AUDIO);

	if(audio_index < 0)
		return 1;

	avlibs_lock->lock("FileAVlibs::write_aframes");

	if(write_pts_base < 0)
	{
		write_pts_base = frames[0]->get_pts();
	}

	stream = context->streams[audio_index];
	audio_ctx = codec_contexts[audio_index];

	if(!resampled_alloc)
	{
		sample_bytes = av_get_bytes_per_sample(audio_ctx->sample_fmt);

		resample_size = AUDIO_BUFFER_SIZE + audio_ctx->frame_size;
		resampled_alloc = av_rescale_rnd(swr_get_delay(swr_ctx,
			asset->streams[streamix].sample_rate) + resample_size,
			audio_ctx->sample_rate, asset->streams[streamix].sample_rate, AV_ROUND_UP);

		if(av_sample_fmt_is_planar(audio_ctx->sample_fmt))
		{
			for(chan = 0; chan < audio_ctx->channels; chan++)
				resampled_data[chan] = new uint8_t[resampled_alloc * sample_bytes];
		}
		else
			resampled_data[0] = new uint8_t[resampled_alloc * sample_bytes * audio_ctx->channels];
	}

// Resample the whole buffer
	for(chan = 0; chan < input_channels; chan++)
		in_data[chan] = frames[chan]->buffer;
	if(av_sample_fmt_is_planar(audio_ctx->sample_fmt))
	{
		for(chan = 0; chan < audio_ctx->channels; chan++)
			resampled_ptr[chan] = resampled_data[chan] + resample_fill * sample_bytes;
	}
	else
		resampled_ptr[0] = resampled_data[0] + resample_fill * sample_bytes * audio_ctx->channels;

	if((resampled_length = swr_convert(swr_ctx, resampled_ptr,
		resampled_alloc - resample_fill, (const uint8_t**)in_data, in_length)) < 0)
	{
		errormsg(_("Failed to resample audio data"));
		avlibs_lock->unlock();
		return 1;
	}

	rv = write_samples(resampled_length + resample_fill, audio_ctx,
		frames[0]->get_pts() - write_pts_base);
	avlibs_lock->unlock();
	return rv;
}

int FileAVlibs::write_samples(int resampled_length, AVCodecContext *audio_ctx,
		ptstime pts)
{
	AVStream *stream;
	int chan, rv, samples_written;
	int frame_size = audio_ctx->frame_size;
	int linesize, frames;

	if(frame_size == 0)
		frame_size = resampled_length;

	avaframe->nb_samples = frame_size;

	av_samples_get_buffer_size(&linesize, audio_ctx->channels,
		frame_size, audio_ctx->sample_fmt, 1);
	frames = resampled_length / frame_size;

	stream = context->streams[audio_index];

	samples_written = 0;

	if(audio_pos == 0)
		audio_pos = pts / av_q2d(audio_ctx->time_base);

	if(!avapkt)
		avapkt = allocate_packet();

	resampled_length -= frame_size;
	samples_written = 0;
	for(int i = 0; i < frames; i++)
	{
		avaframe->pts = audio_pos;

		if(av_sample_fmt_is_planar(audio_ctx->sample_fmt))
		{
			for(chan = 0; chan < audio_ctx->channels; chan++)
				avaframe->data[chan] = &resampled_data[chan][i * frame_size * sample_bytes];
		}
		else
			avaframe->data[0] = &resampled_data[0][i * frame_size * sample_bytes * audio_ctx->channels];

		avaframe->linesize[0] = linesize;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,47,100)
		int got_output;

		if((rv = avcodec_encode_audio2(audio_ctx, avapkt, avaframe, &got_output)) < 0)
		{
			liberror(rv, _("Failed to encode audio frame"));
			return 1;
		}
		audio_pos += frame_size;
		samples_written += frame_size;

		if(got_output)
		{
			avapkt->stream_index = audio_index;
			av_packet_rescale_ts(avapkt, audio_ctx->time_base,
				stream->time_base);

			if((rv = av_interleaved_write_frame(context, avapkt)) < 0)
			{
				liberror(rv, _("Failed to write audio packet"));
				return 1;
			}
		}
#else
		if(rv = av_frame_make_writable(avaframe))
		{
			liberror(rv, _("Could not make audio frame writable"));
			return 1;
		}

		if(rv = avcodec_send_frame(audio_ctx, avaframe))
		{
			liberror(rv, _("Failed to send audio frame"));
			return 1;
		}

		while((rv = avcodec_receive_packet(audio_ctx, avapkt)) >= 0)
		{
			avapkt->stream_index = audio_index;
			av_packet_rescale_ts(avapkt, audio_ctx->time_base,
				stream->time_base);

			if((rv = av_interleaved_write_frame(context, avapkt)) < 0)
			{
				liberror(rv, _("Failed to write audio packet"));
				return 1;
			}
		}

		if(rv != AVERROR(EAGAIN))
		{
			liberror(rv, _("Failed to encode audio packet"));
			return 1;
		}

		audio_pos += frame_size;
		samples_written += frame_size;
#endif

	}

	if(samples_written < resampled_length + frame_size)
	{
		resample_fill = resampled_length + frame_size - samples_written;

		if(av_sample_fmt_is_planar(audio_ctx->sample_fmt))
		{
			for(chan = 0; chan < audio_ctx->channels; chan++)
			{
				memmove(resampled_data[chan], &resampled_data[chan][samples_written * sample_bytes],
					resample_fill * sample_bytes);
			}
		}
		else
			memmove(resampled_data[0],
				&resampled_data[0][samples_written * sample_bytes * audio_ctx->channels],
				resample_fill * sample_bytes * audio_ctx->channels);
	}
	else
		resample_fill = 0;
	return 0;
}

int FileAVlibs::media_seek(int stream_index, int64_t rqpos, AVPacket *pkt, int64_t pktpos)
{
	int res, num;
	int toc_ix;
	stream_item *itm, *nxtitm;

	if(asset->single_image)
	{
		if((res = avformat_seek_file(context, -1,
			INT64_MIN, (int64_t)0, INT64_MAX, AVSEEK_FLAG_ANY)) < 0)
		{
			liberror(res, _("Media seeking failed"));
			return -1;
		}
		avcodec_flush_buffers(codec_contexts[stream_index]);
		file_eof = 0;
		return 0;
	}
	toc_ix = asset->tocfile->id_to_index(stream_index);
	itm = asset->tocfile->get_item(toc_ix, rqpos, &nxtitm);

	if(itm)
	{
		int fl = AVSEEK_FLAG_ANY;

		file_eof = 0;

		// No need to seek forward before next keyframe
		if(nxtitm && pktpos <= nxtitm->index && pktpos > itm->index &&
				rqpos > pktpos)
			return 0;

		if(asset->tocfile->toc_streams[toc_ix].data1 & STRDSC_SEEKBYTES)
			fl |= AVSEEK_FLAG_BYTE;

		avio_flush(context->pb);
		avformat_flush(context);
		av_packet_unref(pkt);
		avcodec_flush_buffers(codec_contexts[stream_index]);

		if((res = avformat_seek_file(context, -1,
			INT64_MIN, itm->offset, itm->offset, fl)) < 0)
		{
			liberror(res, _("Media seeking failed"));
			return -1;
		}
		do
		{
			if((res = av_read_frame(context, pkt)) != 0)
			{
				liberror(res, _("Reading after media seek failed"));
				return -1;
			}
			if(pkt->stream_index != stream_index || pkt->pts != AV_NOPTS_VALUE)
				continue;

		} while(pkt->pts < itm->index);
		return 1;
	}
	return 0;
}

// Callbacks for TOC generation
int FileAVlibs::get_streamcount()
{
	int count = 0;

	for(int i = 0; i < asset->nb_streams; i++)
	{
		if(asset->streams[i].options & (STRDSC_AUDIO | STRDSC_VIDEO))
			count++;
	}
	return count;
}

int FileAVlibs::fill_toc_streams(FileTOC *tocfile)
{
	AVStream *stream;
	AVCodecContext *decoder_context;
	AVPacket *pktp = allocate_packet();
	int trid, trx;
	int posbytes;
	int interrupt = 0;
	int err;
	stream_params *toc;
	int nstreams = 0;

	struct {
		int64_t pts;
		int64_t offs;
		int kf;
	} kf;

	for(trx = 0; trx < asset->nb_streams; trx++)
	{
		if(asset->streams[trx].options & (STRDSC_AUDIO | STRDSC_VIDEO))
			trid = asset->streams[trx].stream_index;
		else
			continue;

		stream = context->streams[trid];
		decoder_context = codec_contexts[trid];
		toc = &tocfile->toc_streams[nstreams++];

		switch(decoder_context->codec_type)
		{
		case AVMEDIA_TYPE_AUDIO:
			toc->type = TOCSTREAM_AUDIO;
			toc->rate_num = asset->streams[trx].sample_rate;
			toc->rate_denom = 1;
			break;

		case AVMEDIA_TYPE_VIDEO:
			toc->type = TOCSTREAM_VIDEO;
			toc->rate_num = stream->r_frame_rate.num;
			toc->rate_denom = stream->r_frame_rate.den;
			break;
		}
		toc->id = trid;
		toc->layers = asset->streams[trx].channels;
		toc->data1 = posbytes;
		toc->data2 = 0;
		toc->min_index = INT64_MAX;
		toc->min_offset = INT64_MAX;
		toc->max_index = -INT64_MAX;
		toc->max_offset = -INT64_MAX;
	}
	avformat_seek_file(context, -1, INT64_MIN, INT64_MIN, 0,
		AVSEEK_FLAG_ANY);
	avformat_flush(context);

	kf.kf = 0;
	while((err = av_read_frame(context, pktp)) == 0)
	{
		trid = pktp->stream_index;
		for(trx = 0; trx < nstreams; trx++)
		{
			if(tocfile->toc_streams[trx].id == trid)
				break;
		}
		if(trx >= nstreams)
			continue;

		toc = &tocfile->toc_streams[trx];

		if(pktp->flags & AV_PKT_FLAG_KEY)
		{
			kf.pts = AV_NOPTS_VALUE;
			if(pktp->pts != AV_NOPTS_VALUE)
				kf.pts = pktp->pts;
			else if(pktp->dts != AV_NOPTS_VALUE)
				kf.pts = pktp->dts;
			if(kf.pts != AV_NOPTS_VALUE)
			{
				kf.kf = 1;
				kf.offs = AV_NOPTS_VALUE;
			}
		}

		if(kf.kf)
		{
			if(posbytes)
			{
				if(pktp->pos != 1)
					kf.offs = pktp->pos;
			}
			else if(pktp->dts != AV_NOPTS_VALUE)
				kf.offs = pktp->dts;

			if(pktp->pts != AV_NOPTS_VALUE && pktp->pts > toc->max_index)
				toc->max_index = pktp->pts;
		}

		if(posbytes)
		{
			if(pktp->pos != -1 && pktp->pos > toc->max_offset)
				toc->max_offset = pktp->pos + pktp->size;
		}
		else if(pktp->dts != AV_NOPTS_VALUE && pktp->dts > toc->max_offset)
			toc->max_offset = pktp->dts + pktp->duration;

		if(pktp->dts != AV_NOPTS_VALUE);
			toc->data2++;      // number of frames

		if(kf.kf && kf.offs != AV_NOPTS_VALUE)
		{
			interrupt = asset->tocfile->append_item(trx, kf.pts,
				kf.offs, pktp->pos);
			kf.kf = 0;
			if(toc->min_index > kf.pts)
				toc->min_index = kf.pts;
			if(toc->min_offset > kf.offs)
				toc->min_offset = kf.offs;
		}
		if(interrupt)
		{
			deallocate_packet(&pktp);
			return 1;
		}
	}

	deallocate_packet(&pktp);

	if(err != AVERROR_EOF)
		return 1;

	return 0;
}

void FileAVlibs::versionifo(int indent)
{
#define PRINT_LIB_INFO(libname, LIBNAME) \
	{ \
		unsigned int version = libname##_version(); \
		fprintf(stderr, "%*slib%-11s %2d.%3d.%3d / %2d.%3d.%3d%s\n", \
			indent, "", #libname, \
			LIB##LIBNAME##_VERSION_MAJOR, \
			LIB##LIBNAME##_VERSION_MINOR, \
			LIB##LIBNAME##_VERSION_MICRO, \
			version >> 16, \
			version >> 8 & 0xff, \
			version & 0xff, \
			version == LIB##LIBNAME##_VERSION_INT ? "" : " mismatch"); \
	}
#ifdef LIBAVUTIL_VERSION
	PRINT_LIB_INFO(avutil, AVUTIL);
#endif
#ifdef LIBAVCODEC_VERSION
	PRINT_LIB_INFO(avcodec, AVCODEC);
#endif
#ifdef LIBAVFORMAT_VERSION
	PRINT_LIB_INFO(avformat, AVFORMAT);
#endif
#ifdef LIBAVDEVICE_VERSION
	PRINT_LIB_INFO(avdevice, AVDEVICE);
#endif
#ifdef LIBAVFILTER_VERSION
	PRINT_LIB_INFO(avfilter, AVFILTER);
#endif
#ifdef LIBAVRESAMPLE_VERSION
	PRINT_LIB_INFO(avresample, AVRESAMPLE);
#endif
#ifdef LIBSWSCALE_VERSION
	PRINT_LIB_INFO(swscale,  SWSCALE);
#endif
#ifdef LIBSWRESAMPLE_VERSION
	PRINT_LIB_INFO(swresample, SWRESAMPLE);
#endif
#ifdef LIBPOSTPROC_VERSION
	PRINT_LIB_INFO(postproc, POSTPROC);
#endif
}

void FileAVlibs::get_parameters(BC_WindowBase *parent_window,
	Asset *asset,
	BC_WindowBase* &format_window,
	int options)
{
	Paramlist *defaults, *codecs;
	int rs, rsc, rsp;
	AVlibsConfig *window = 0;
	int stream;

	FileAVlibs::avlibs_lock->lock("FileAVlibs::get_parameters");
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,10,100)
	avcodec_register_all();
#endif
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
	av_register_all();
#endif
	FileAVlibs::avlibs_lock->unlock();
	rs = 1;
	if(codecs = scan_codecs(output_format(asset->format), asset, options))
	{
		window = new AVlibsConfig(asset, codecs, options);
		format_window = window;
		rs = window->run_window();
	}

	if(!rs && window->codecopts)
	{
		Paramlist **alist, **plist;
		Param *pa;
		int rsc, rsp;

		rsc = window->codecthread->win_result;
		rsp = window->privthread->win_result;
		alist = plist = 0;
		defaults = scan_encoder_opts((AVCodecID)window->current_codec, options);
		window->codecopts->remove_equiv(defaults);

		if(options & SUPPORTS_VIDEO)
		{
			if((pa = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(AVL_PARAM_CODEC_VIDEO)) &&
					pa->intvalue != window->current_codec)
			{
				pa->type |= PARAMTYPE_CHNG;
				rsc = rsp = 0;
			}
			pa = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->set(AVL_PARAM_CODEC_VIDEO, window->current_codec);
			pa->set(window->codecopts->name);
			if((stream = asset->get_stream_ix(STRDSC_VIDEO)) >= 0)
				strcpy(asset->streams[stream].codec, window->codecopts->name);
			if(!rsc)
				alist = &asset->encoder_parameters[FILEAVLIBS_VCODEC_IX];
			if(!rsp)
				plist = &asset->encoder_parameters[FILEAVLIBS_VPRIVT_IX];
		}
		else if(options & SUPPORTS_AUDIO)
		{
			if((pa = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(AVL_PARAM_CODEC_AUDIO)) &&
					pa->intvalue != window->current_codec)
			{
				pa->type |= PARAMTYPE_CHNG;
				rsc = rsp = 0;
			}
			pa = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->set(AVL_PARAM_CODEC_AUDIO, window->current_codec);
			pa->set(window->codecopts->name);
			if((stream = asset->get_stream_ix(STRDSC_AUDIO)) >= 0)
				strcpy(asset->streams[stream].codec, window->codecopts->name);
			if(!rsc)
				alist = &asset->encoder_parameters[FILEAVLIBS_ACODEC_IX];
			if(!rsp)
				plist = &asset->encoder_parameters[FILEAVLIBS_APRIVT_IX];
		}
		update_codeclist(asset, codecs, options);

		if(alist && *alist)
		{
			delete *alist;
			*alist = 0;
		}
		if(plist && *plist)
		{
			delete *plist;
			*plist = 0;
		}
		if(alist && window->codecopts->total() > 0)
		{
			window->codecopts->clean_list();
			*alist = window->codecopts;
			window->codecopts = 0;
		}
		delete defaults;

		if(plist && window->codec_private)
		{
			defaults = scan_encoder_private_opts((AVCodecID)window->current_codec, options);
			window->codec_private->remove_equiv(defaults);
			window->codec_private->clean_list();
			if(window->codec_private->total() > 0)
				*plist = window->codec_private;
			window->codec_private = 0;
			delete defaults;
		}
		if(asset->format == FILE_YUV && window->pipeconfig)
		{
			strcpy(asset->pipe, window->pipeconfig->pipe_textbox->get_text());
			asset->use_pipe = window->pipeconfig->pipe_checkbox->get_value();
			window->pipeconfig->save_defaults();
		}
	}
	delete window;
}

void FileAVlibs::restore_codec_options(Paramlist *codecs)
{
	Param *apar;
	Paramlist *list;

	if(!codecs)
		return;

	for(apar = codecs->first; apar; apar = apar->next)
	{
		if(list = apar->subparams)
		{
			for(Param *p = list->first; p; p = p->next)
			{
				for(int i = 0; encoder_params[i].name; i++)
				{
					if(strcmp(encoder_params[i].name, p->name) == 0)
					{
						switch(encoder_params[i].type)
						{
						case PARAMTYPE_INT:
							p->set(atoi(p->stringvalue));
							break;
						case PARAMTYPE_LNG:
#if __WORDSIZE == 64
							p->set(atol(p->stringvalue));
#else
							p->set(atoll(p->stringvalue));
#endif
							break;
						case PARAMTYPE_DBL:
							p->set(atof(p->stringvalue));
							break;
						}
						// Delete converted string
						p->set_string(0);
						p->type &= ~PARAMTYPE_STR;
						break;
					}
				}
			}
		}
	}
}

void FileAVlibs::deserialize_params(Asset *asset)
{
	if(!(FileAVlibs::encoder_formatname(asset->format)))
		return;

	restore_codec_options(asset->encoder_parameters[FILEAVLIBS_CODECS_IX]);
}

void FileAVlibs::load_render_options(Asset *asset)
{
	const char *name;
	AVOutputFormat *oformat = 0;
	AVCodec *encoder;
	Param *apar;
	Paramlist *list;
	int stream;

	if(!(name = FileAVlibs::encoder_formatname(asset->format)))
		return;
	asset->encoder_parameters[FILEAVLIBS_FORMAT_IX] = AVlibsConfig::load_options(asset, FILEAVLIBS_FORMAT_CONFIG, name);
	asset->encoder_parameters[FILEAVLIBS_CODECS_IX] = AVlibsConfig::load_options(asset, FILEAVLIBS_CODECS_CONFIG, name);

	restore_codec_options(asset->encoder_parameters[FILEAVLIBS_CODECS_IX]);

	FileAVlibs::avlibs_lock->lock("AVlibsConfig::AVlibsConfig");
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,10,100)
	avcodec_register_all();
#endif
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
	av_register_all();
#endif
	FileAVlibs::avlibs_lock->unlock();

	if(oformat = av_guess_format(name, 0, 0))
	{
		Paramlist *codecs = asset->encoder_parameters[FILEAVLIBS_CODECS_IX];

		if(!codecs)
		{
			asset->encoder_parameters[FILEAVLIBS_CODECS_IX] =
				codecs = new Paramlist("codecs");
		}
		apar = 0;

		if((stream = asset->get_stream_ix(STRDSC_AUDIO)) >= 0)
			asset->streams[stream].codec[0] = 0;

		if(stream < 0 || oformat->audio_codec == AV_CODEC_ID_NONE)
			codecs->remove_param(AVL_PARAM_CODEC_AUDIO);
		else if(!(apar = codecs->find(AVL_PARAM_CODEC_AUDIO)) ||
				!(encoder = avcodec_find_encoder((AVCodecID)apar->intvalue)))
		{
			if(encoder = avcodec_find_encoder(oformat->audio_codec))
			{
				apar = codecs->set(AVL_PARAM_CODEC_AUDIO, oformat->audio_codec);
				apar->set(encoder->name);
				Paramlist *cdc = scan_codecs(oformat, asset, SUPPORTS_AUDIO);
				update_codeclist(asset, cdc, SUPPORTS_AUDIO);
				delete cdc;
			}
		}
		if(apar && stream >= 0)
		{
			strncpy(asset->streams[stream].codec, encoder->name, MAX_LEN_CODECNAME);
			asset->streams[stream].codec[MAX_LEN_CODECNAME - 1] = 0;
			asset->encoder_parameters[FILEAVLIBS_ACODEC_IX] =
				AVlibsConfig::load_options(asset,
				FILEAVLIBS_ACODEC_CONFIG,
				asset->streams[stream].codec);
			asset->encoder_parameters[FILEAVLIBS_APRIVT_IX] =
				AVlibsConfig::load_options(asset,
				FILEAVLIBS_APRIVT_CONFIG,
				asset->streams[stream].codec);
		}

		apar = 0;
		if((stream = asset->get_stream_ix(STRDSC_VIDEO)) >= 0)
			asset->streams[stream].codec[0] = 0;
		if(stream < 0 || oformat->video_codec == AV_CODEC_ID_NONE)
			codecs->remove_param(AVL_PARAM_CODEC_VIDEO);
		else if(!(apar = codecs->find(AVL_PARAM_CODEC_VIDEO)) ||
				!(encoder = avcodec_find_encoder((AVCodecID)apar->intvalue)))
		{
			if(encoder = avcodec_find_encoder(oformat->video_codec))
			{
				apar = codecs->set(AVL_PARAM_CODEC_VIDEO, oformat->video_codec);
				apar->set(encoder->name);
				Paramlist *cdc = scan_codecs(oformat, asset, SUPPORTS_VIDEO);
				update_codeclist(asset, cdc, SUPPORTS_VIDEO);
				delete cdc;
			}
		}
		if(apar && stream >= 0)
		{
			strncpy(asset->streams[stream].codec, encoder->name, MAX_LEN_CODECNAME);
			asset->streams[stream].codec[MAX_LEN_CODECNAME - 1] = 0;
			asset->encoder_parameters[FILEAVLIBS_VCODEC_IX] =
				AVlibsConfig::load_options(asset,
				FILEAVLIBS_VCODEC_CONFIG,
				asset->streams[stream].codec);
			asset->encoder_parameters[FILEAVLIBS_VPRIVT_IX] =
				AVlibsConfig::load_options(asset,
				FILEAVLIBS_VPRIVT_CONFIG,
				asset->streams[stream].codec);
		}
	}
	if(mwindow_global)
	{
		if(!asset->encoder_parameters[FILEAVLIBS_METADT_IX])
			asset->encoder_parameters[FILEAVLIBS_METADT_IX] = new Paramlist("metadata");
		list = asset->encoder_parameters[FILEAVLIBS_METADT_IX];
		if(edlsession->metadata_copyright[0])
			list->set("copyright", edlsession->metadata_copyright);
		else
			list->remove_param("copyright");
		if(edlsession->metadata_title[0])
			list->set("title", edlsession->metadata_title);
		else
			list->remove_param("title");
		if(edlsession->metadata_author[0])
			list->set("author", edlsession->metadata_author);
		else
			list->remove_param("author");
		if(edlsession->experimental_codecs)
			list->set("strict", "-2");
		else
			list->remove_param("strict");
	}
}

void FileAVlibs::get_format_params(Asset *asset, int options)
{
	Paramlist *glob, *fmt;

	if(!(FileAVlibs::encoder_formatname(asset->format)))
		return;

	glob = FileAVlibs::scan_global_options(options);
	if(fmt = FileAVlibs::scan_format_options(asset->format, options))
	{
		fmt->copy_values(asset->encoder_parameters[FILEAVLIBS_FORMAT_IX]);
		fmt->join_list(glob);
	}
	else
		fmt = glob;
	fmt->store_defaults();
	delete asset->encoder_parameters[ASSET_FMT_IX];
	asset->encoder_parameters[ASSET_FMT_IX] = fmt;
}

void FileAVlibs::set_format_params(Asset *asset)
{
	Paramlist *glob, *fmt;

	if(!(FileAVlibs::encoder_formatname(asset->format)))
		return;

	if(!asset->encoder_parameters[ASSET_FMT_IX])
		return;
	glob = FileAVlibs::scan_global_options(SUPPORTS_AUDIO|SUPPORTS_VIDEO);
	fmt = FileAVlibs::scan_format_options(asset->format,
		SUPPORTS_AUDIO|SUPPORTS_VIDEO);
	fmt->join_list(glob);
	asset->encoder_parameters[ASSET_FMT_IX]->remove_equiv(fmt);
	delete asset->encoder_parameters[FILEAVLIBS_FORMAT_IX];

	if(asset->encoder_parameters[ASSET_FMT_IX]->total() > 0)
	{
		asset->encoder_parameters[ASSET_FMT_IX]->clean_list();
		asset->encoder_parameters[FILEAVLIBS_FORMAT_IX] =
			asset->encoder_parameters[ASSET_FMT_IX];
	}
	else
	{
		delete asset->encoder_parameters[ASSET_FMT_IX];
		asset->encoder_parameters[FILEAVLIBS_FORMAT_IX] = 0;
	}
	asset->encoder_parameters[ASSET_FMT_IX] = 0;
}

void FileAVlibs::get_decoder_format_defaults(Asset *asset, AVFormatContext *ctx)
{
	Paramlist *glob, *fmt;

	glob = scan_options(ctx->av_class,
		SUPPORTS_DECODER, ContainerSelection::container_extension(asset->format));

	if(ctx->iformat)
	{
		fmt = scan_options(ctx->iformat->priv_class, SUPPORTS_DECODER,
			ContainerSelection::container_extension(asset->format));
		fmt->join_list(glob);
	}
	else
		fmt = glob;

	fmt->store_defaults();
	asset->decoder_parameters[ASSET_DFORMAT_IX] = fmt;
	// Make a clean copy of parameters
	if(fmt)
	{
		glob = new Paramlist(fmt->name);
		glob->copy_top_level(fmt);
		delete asset->decoder_parameters[ASSET_DFDEFLT_IX];
		asset->decoder_parameters[ASSET_DFDEFLT_IX] = glob;
	}
}

void FileAVlibs::update_decoder_format_defaults(Asset *asset)
{
	if(asset->decoder_parameters[ASSET_DFORMAT_IX] &&
			asset->decoder_parameters[FILEAVLIBS_DFORMAT_IX])
		asset->decoder_parameters[ASSET_DFORMAT_IX]->copy_values(
			asset->decoder_parameters[FILEAVLIBS_DFORMAT_IX]);

	for(int i = 0; i < asset->nb_streams; i++)
	{
		if(asset->streams[i].decoding_params[ASSET_DFORMAT_IX] &&
				asset->streams[i].decoding_params[FILEAVLIBS_DFORMAT_IX])
			asset->streams[i].decoding_params[ASSET_DFORMAT_IX]->copy_values(
				asset->streams[i].decoding_params[FILEAVLIBS_DFORMAT_IX]);
	}
}

void FileAVlibs::set_decoder_format_parameters(Asset *asset)
{
	Paramlist *list;

	if(!asset->decoder_parameters[ASSET_DFORMAT_IX] ||
			!asset->decoder_parameters[ASSET_DFDEFLT_IX])
		return;

	delete asset->decoder_parameters[FILEAVLIBS_DFORMAT_IX];

	list = new Paramlist(asset->decoder_parameters[ASSET_DFORMAT_IX]->name);
	list->copy_from(asset->decoder_parameters[ASSET_DFORMAT_IX]);

	list->remove_equiv(asset->decoder_parameters[ASSET_DFDEFLT_IX]);

	if(list->total() > 0)
	{
		list->clean_list();
		asset->decoder_parameters[FILEAVLIBS_DFORMAT_IX] = list;
	}
	else
	{
		delete list;
		asset->decoder_parameters[FILEAVLIBS_DFORMAT_IX] = 0;
	}
}

void FileAVlibs::set_stream_decoder_parameters(struct streamdesc *stream)
{
	Paramlist *list;

	if(!stream->decoding_params[ASSET_DFORMAT_IX] ||
		!stream->decoding_params[ASSET_DFDEFLT_IX])
		return;

	delete stream->decoding_params[FILEAVLIBS_DFORMAT_IX];

	list = new Paramlist(stream->decoding_params[ASSET_DFDEFLT_IX]->name);
	list->copy_from(stream->decoding_params[ASSET_DFORMAT_IX]);
	list->remove_equiv(stream->decoding_params[ASSET_DFDEFLT_IX]);

	if(list->total() > 0)
	{
		list->clean_list();
		stream->decoding_params[FILEAVLIBS_DFORMAT_IX] = list;
	}
	else
	{
		delete list;
		stream->decoding_params[FILEAVLIBS_DFORMAT_IX] = 0;
	}
}

Paramlist *FileAVlibs::get_decoder_params(AVCodec *codec)
{
	Paramlist *pl;

	if(codec->priv_class)
	{
		pl = scan_options(codec->priv_class, SUPPORTS_DECODER, codec->name);
		if(pl->total())
		{
			pl->store_defaults();
			return pl;
		}
		delete pl;
	}
	return 0;
}

void FileAVlibs::save_render_options(Asset *asset)
{
	const char *name;
	Paramlist *tmp;
	int stream;

	if(!(name = FileAVlibs::encoder_formatname(asset->format)))
		return;

	AVlibsConfig::save_encoder_options(asset, FILEAVLIBS_FORMAT_IX,
		FILEAVLIBS_FORMAT_CONFIG, name);
	AVlibsConfig::save_encoder_options(asset, FILEAVLIBS_CODECS_IX,
		FILEAVLIBS_CODECS_CONFIG, name);
	if((stream = asset->get_stream_ix(STRDSC_AUDIO)) >= 0)
	{
		AVlibsConfig::save_encoder_options(asset, FILEAVLIBS_ACODEC_IX,
			FILEAVLIBS_ACODEC_CONFIG, asset->streams[stream].codec);
		AVlibsConfig::save_encoder_options(asset, FILEAVLIBS_APRIVT_IX,
			FILEAVLIBS_APRIVT_CONFIG, asset->streams[stream].codec);
	}
	if((stream = asset->get_stream_ix(STRDSC_VIDEO)) >= 0)
	{
		AVlibsConfig::save_encoder_options(asset, FILEAVLIBS_VCODEC_IX,
			FILEAVLIBS_VCODEC_CONFIG, asset->streams[stream].codec);
		AVlibsConfig::save_encoder_options(asset, FILEAVLIBS_VPRIVT_IX,
			FILEAVLIBS_VPRIVT_CONFIG, asset->streams[stream].codec);
	}
}

int FileAVlibs::update_codeclist(Asset *asset, Paramlist *codecs, int options)
{
	int changed = 0;
	const char *cdkn = options & SUPPORTS_VIDEO ? AVL_PARAM_CODEC_VIDEO :
			AVL_PARAM_CODEC_AUDIO;

	if(!codecs)
		return 0;

	if(Param *p = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(cdkn))
	{
		Param *q = codecs->find(p->stringvalue);
		changed = p->type;

		if(p->type & PARAMTYPE_CHNG)
		{
			delete p->subparams;
			p->subparams = 0;
			p->type &= ~PARAMTYPE_CHNG;
		}
		if(q && q->subparams)
		{
			p->add_subparams(cdkn);
			p->type |= PARAMTYPE_CODK;

			for(Param *r = q->subparams->first; r; r = r->next)
			{
				Paramlist *rs = r->subparams;

				changed |= rs->type;
				if(rs->type & PARAMTYPE_INT)
					p->subparams->set(r->name, rs->selectedint);
				if(rs->type & PARAMTYPE_LNG)
					p->subparams->set(r->name, rs->selectedlong);
				if(rs->type & PARAMTYPE_DBL)
					p->subparams->set(r->name, rs->selectedfloat);
			}
		}
	}
	return changed & PARAMTYPE_CHNG;
}


Paramlist *FileAVlibs::scan_global_options(int options)
{
	return scan_options(avformat_get_class(), 0, "AVlibsGlobal");
}

AVOutputFormat *FileAVlibs::output_format(int format)
{
	const char *name = encoder_formatname(format);

	if(name)
		return av_guess_format(name, 0, 0);

	return 0;
}

Paramlist *FileAVlibs::scan_format_options(int format, int options)
{
	AVOutputFormat *oformat;
	const char *name = encoder_formatname(format);

	if(name && (oformat = FileAVlibs::output_format(format)))
		return scan_options(oformat->priv_class, 0, name);

	return 0;
}

Paramlist *FileAVlibs::scan_options(const AVClass *avclass, int options, const char *listname)
{
	int typefl;
	const AVOption *opt = 0;
	Param *param;
	Paramlist *list;

	list = new Paramlist(listname);

	if(options & SUPPORTS_DECODER)
		typefl = AV_OPT_FLAG_DECODING_PARAM;
	else
		typefl = AV_OPT_FLAG_ENCODING_PARAM;

	if(options & SUPPORTS_AUDIO)
		typefl |= AV_OPT_FLAG_AUDIO_PARAM;
	if(options & SUPPORTS_VIDEO)
		typefl |= AV_OPT_FLAG_VIDEO_PARAM;

	while(opt = av_opt_next(&avclass, opt))
	{
		if(opt->type == AV_OPT_TYPE_CONST || skip_avoption(opt, typefl))
			continue;
		param = opt2param(list, opt);
		if(param && opt->unit)
		{
			const AVOption *subopt = 0;
			Paramlist *sublist = param->add_subparams(opt->name);

			while(subopt = av_opt_next(&avclass, subopt))
			{
				if(subopt->type == AV_OPT_TYPE_CONST &&
						!skip_avoption(subopt, typefl) &&
						!strcmp(subopt->unit, opt->unit))
					opt2param(sublist, subopt);
			}
		}
	}
	return list;
}

Paramlist *FileAVlibs::clean_list(Paramlist *list)
{
	Param *current;

	for(current = list->first; current; current = current->next)
	{
		while(current && current->subparams && (current->subparams->total() <= 1))
		{
			Param *np = current->next;
			delete current;
			current = np;
		}
		if(!current)
			break;
	}
	return list;
}

Param *FileAVlibs::opt2param(Paramlist *list, const AVOption *opt)
{
	Param *param = 0;

	switch(opt->type)
	{
	case AV_OPT_TYPE_FLAGS:
		param = list->append_param(opt->name, (int64_t)opt->default_val.i64);
		param->type |= PARAMTYPE_BITS;
		break;
	case AV_OPT_TYPE_INT:
	case AV_OPT_TYPE_INT64:
	case AV_OPT_TYPE_CONST:
	case AV_OPT_TYPE_DURATION:
	case AV_OPT_TYPE_PIXEL_FMT:
	case AV_OPT_TYPE_SAMPLE_FMT:
		param = list->append_param(opt->name, (int64_t)opt->default_val.i64);
		break;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(55, 17, 103)
	case AV_OPT_TYPE_BOOL:
		param = list->append_param(opt->name, (int64_t)opt->default_val.i64);
		param->type |= PARAMTYPE_BOOL;
		break;
#endif
	case AV_OPT_TYPE_DOUBLE:
	case AV_OPT_TYPE_FLOAT:
		param = list->append_param(opt->name, opt->default_val.dbl);
		break;
	case AV_OPT_TYPE_STRING:
	case AV_OPT_TYPE_IMAGE_SIZE:
	case AV_OPT_TYPE_VIDEO_RATE:
	case AV_OPT_TYPE_COLOR:
		param = list->append_param(opt->name, opt->default_val.str);
		break;
	default:
		break;
	}
	if(param)
		param->set_help(opt->help);

	return param;
}

int FileAVlibs::skip_avoption(const AVOption *opt, int typefl)
{
	if(typefl && ((opt->flags & typefl) != typefl))
		return 1;
	if(opt->help && strstr(opt->help, "deprecated"))
		return 1;
	for(int i = 0;;i++)
	{
		if(!ignored[i])
			break;
		if(!strcmp(opt->name, ignored[i]))
			return 1;
	}
	return 0;
}

void FileAVlibs::fill_encoder_params(Paramlist *codecs, AVCodecID codec_id,
    Asset *asset, int options)
{
	AVCodec *encoder;
	Param *param, *sbp;
	int stream;

	if(encoder = avcodec_find_encoder(codec_id))
	{
		if(options & SUPPORTS_AUDIO && encoder->type != AVMEDIA_TYPE_AUDIO)
			return;
		if(options & SUPPORTS_VIDEO && encoder->type != AVMEDIA_TYPE_VIDEO)
			return;
		if(codecs->find(encoder->name))
			return;
		if(!edlsession->experimental_codecs &&
				encoder->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
			return;
		param = codecs->append_param(encoder->name, codec_id);
		param->set_help(encoder->long_name);
		// Additional parameters
		if(encoder->type == AVMEDIA_TYPE_VIDEO &&
				(stream = asset->get_stream_ix(STRDSC_VIDEO)) >= 0)
		{
			Paramlist *encparams = param->add_subparams(encoder->name);

			if(encoder->pix_fmts)
			{
				const AVPixelFormat *pix_fmt;

				sbp = encparams->append_param(encoder_params[ENC_PIX_FMTS].name,
					(int)encoder->pix_fmts[0]);
				Paramlist *sublist = sbp->add_subparams(encoder_params[ENC_PIX_FMTS].name);
				sublist->set_selected((int)encoder->pix_fmts[0]);

				for(pix_fmt = encoder->pix_fmts; *pix_fmt != AV_PIX_FMT_NONE; pix_fmt++)
				{
					Param *p = sublist->append_param("", *pix_fmt);
					strcpy(p->name, av_get_pix_fmt_name(*pix_fmt));
				}
			}
			if(encoder->supported_framerates)
			{
				const AVRational *framerate;

				sbp = encparams->append_param(encoder_params[ENC_FRAMERATES].name,
					av_q2d(encoder->supported_framerates[0]));
				Paramlist *sublist = sbp->add_subparams(encoder_params[ENC_FRAMERATES].name);
				sublist->type |= PARAMTYPE_HIDN;

				for(framerate = encoder->supported_framerates; framerate->num != 0; framerate++)
				{
					Param *p = sublist->append_param("", av_q2d(*framerate));
					sprintf(p->name, "%d/%d", framerate->num, framerate->den);
				}
			}
		}
		if(encoder->type == AVMEDIA_TYPE_AUDIO &&
				(stream = asset->get_stream_ix(STRDSC_VIDEO)) >= 0)
		{
			Paramlist *encparams = param->add_subparams(encoder->name);

			if(encoder->supported_samplerates)
			{
				const int *srate;
				sbp = encparams->append_param(encoder_params[ENC_SAMPLERATES].name,
					encoder->supported_samplerates[0]);
				Paramlist *sublist = sbp->add_subparams(encoder_params[ENC_SAMPLERATES].name);
				sublist->type |= PARAMTYPE_HIDN;

				for(srate = encoder->supported_samplerates; *srate; srate++)
				{
					Param *p = sublist->append_param("", *srate);
					sprintf(p->name, "%d", *srate);
				}
			}
			if(encoder->sample_fmts)
			{
				const AVSampleFormat *sfmt;

				sbp = encparams->append_param(encoder_params[ENC_SAMPLE_FMTS].name,
					encoder->sample_fmts[0]);
				Paramlist *sublist = sbp->add_subparams(encoder_params[ENC_SAMPLE_FMTS].name);
				sublist->set_selected(encoder->sample_fmts[0]);

				for(sfmt = encoder->sample_fmts; *sfmt != AV_SAMPLE_FMT_NONE; sfmt++)
				{
					Param *p = sublist->append_param("", *sfmt);
					strcpy(p->name, av_get_sample_fmt_name(*sfmt));
				}
			}
			if(encoder->channel_layouts)
			{
				const uint64_t *layout;
				int channels = asset->streams[stream].channels;

				sbp = encparams->append_param(encoder_params[ENC_LAYOUTS].name,
					(int64_t)encoder->channel_layouts[0]);
				Paramlist *sublist = sbp->add_subparams(encoder_params[ENC_LAYOUTS].name);

				if(channels > 0 && channels < NUM_INTERNAL_LAYOUTS)
					sublist->set_selected((int64_t)internal_layouts[channels]);
				else
					sublist->set_selected((int64_t)encoder->channel_layouts[0]);

				for(layout = encoder->channel_layouts; *layout; layout++)
				{
					Param *p = sublist->append_param("", (int64_t)*layout);
					av_get_channel_layout_string(p->name, PARAM_NAMELEN,
						av_get_channel_layout_nb_channels(*layout), *layout);
				}
			}
		}
	}
}

Paramlist *FileAVlibs::scan_codecs(AVOutputFormat *oformat, Asset *asset, int options)
{
	Param *param, *sbp;
	Paramlist *codecs = new Paramlist("AVLibCodecs");
	const AVCodecDescriptor *desc = 0;
	int vstream = asset->get_stream_ix(STRDSC_VIDEO);
	int astream = asset->get_stream_ix(STRDSC_AUDIO);

	if(oformat->codec_tag)
	{
		while(desc = avcodec_descriptor_next(desc))
		{
			if((options & SUPPORTS_VIDEO) && desc->type != AVMEDIA_TYPE_VIDEO)
				continue;
			if((options & SUPPORTS_AUDIO) && desc->type != AVMEDIA_TYPE_AUDIO)
				continue;
			if(av_codec_get_tag(oformat->codec_tag, desc->id))
				fill_encoder_params(codecs, desc->id, asset, options);
		}
	}
	else
	{
		AVCodecID id;

		if(options & SUPPORTS_VIDEO)
			id = oformat->video_codec;
		else if(options & SUPPORTS_AUDIO)
			id = oformat->audio_codec;
		fill_encoder_params(codecs, id, asset, options);
	}
	// Remove unusable codecs
	for(param = codecs->first; param; param = param->next)
	{
		if(options & SUPPORTS_VIDEO && vstream >= 0 &&
				asset->streams[vstream].frame_rate)
		{
			while(param && param->subparams &&
				(sbp = param->subparams->find(encoder_params[ENC_FRAMERATES].name)))
			{
				if(!sbp->subparams->find_value(asset->streams[vstream].frame_rate))
				{
					Param *np = param->next;
					delete param;
					param = np;
				}
				else
					break;
			}
		}
		if(options & SUPPORTS_AUDIO && astream >= 0 &&
				asset->streams[astream].sample_rate)
		{
			while(param && param->subparams &&
				(sbp = param->subparams->find(encoder_params[ENC_SAMPLERATES].name)))
			{
				if(!sbp->subparams->find_value(asset->streams[astream].sample_rate))
				{
					Param *np = param->next;
					delete param;
					param = np;
				}
				else
					break;
			}
		}
		if(!param)
			break;
	}
	if(codecs->first)
		codecs->set_selected(codecs->first->intvalue);
	else
	{
		// No suitable codecs
		delete codecs;
		codecs = 0;
	}
	return codecs;
}

const char *FileAVlibs::enc_prompt(const char *enc_name)
{
	for(int i = 0; encoder_params[i].name; i++)
		if(strcmp(encoder_params[i].name, enc_name) == 0)
			return encoder_params[i].prompt;
	return "";
}

Paramlist *FileAVlibs::scan_encoder_opts(AVCodecID codec, int options)
{
	AVCodec *encoder;
	AVCodecContext *ctx;
	Paramlist *libopts = 0;
	int rv;

	if(!(encoder = avcodec_find_encoder(codec)))
		return 0;
	ctx = avcodec_alloc_context3(encoder);
	if(ctx  && ctx->av_class)
		libopts = scan_options(ctx->av_class, options, encoder->name);
	avcodec_free_context(&ctx);
	return clean_list(libopts);
}

Paramlist *FileAVlibs::scan_encoder_private_opts(AVCodecID codec, int options)
{
	AVCodec *encoder;

	encoder = avcodec_find_encoder(codec);

	if(encoder && encoder->priv_class)
		return clean_list(scan_options(encoder->priv_class,
			options, encoder->name));

	return 0;
}

const char *FileAVlibs::dump_AVMediaType(enum AVMediaType type)
{
	switch(type)
	{
	case AVMEDIA_TYPE_UNKNOWN:
		return "Unknown";

	case AVMEDIA_TYPE_VIDEO:
		return "Video";

	case AVMEDIA_TYPE_AUDIO:
		return "Audio";

	case AVMEDIA_TYPE_DATA:
		return "Data";

	case AVMEDIA_TYPE_SUBTITLE:
		return "Subtitle";

	case AVMEDIA_TYPE_ATTACHMENT:
		return "Attachment";

	case AVMEDIA_TYPE_NB:
		return "nb";
	}
	return 0;
}

void FileAVlibs::dump_AVCodec(const AVCodec *codec, int indent)
{
	printf("%*sAVCodec %p dump:\n", indent, "", codec);
	indent += 2;
	printf("%*sid %d type %s name '%s'\n", indent, "", codec->id,
		dump_AVMediaType(codec->type), codec->name);
	printf("%*slong_name '%s'\n", indent, "", codec->long_name);
	printf("%*scapabilities %#x\n", indent, "", codec->capabilities);
	if(codec->supported_framerates)
	{
		printf("%*sSupported framerates:\n", indent, "");
		for(int i = 0; codec->supported_framerates[i].num; i++)
			printf("%*s%s\n", indent + 2, "",
				FileAVlibs::dump_AVRational(&codec->supported_framerates[i]));
	}
	if(codec->pix_fmts)
	{
		printf("%*sSupported pixel formats:\n", indent, "");
		for(int i = 0; codec->pix_fmts[i] != -1; i++)
			printf("%*s%s\n", indent + 2, "",
				av_get_pix_fmt_name(codec->pix_fmts[i]));
	}
	if(codec->supported_samplerates)
	{
		printf("%*sSupported samplerates:\n", indent, "");
		for(int i = 0; codec->supported_samplerates[i]; i++)
			printf("%*s%d\n", indent + 2, "",
				codec->supported_samplerates[i]);
	}
	if(codec->sample_fmts)
	{
		printf("%*sSupported sampleformats:\n", indent, "");
		for(int i = 0; codec->sample_fmts[i] != -1; i++)
			printf("%*s%s\n", indent + 2, "",
				av_get_sample_fmt_name(codec->sample_fmts[i]));
	}
	if(codec->channel_layouts)
	{
		printf("%*sChannel layouts:\n", indent, "");
		for(int i = 0; codec->channel_layouts[i]; i++)
			printf("%*s%#" PRIx64 "\n", indent + 2, "",
				codec->channel_layouts[i]);
	}
	printf("%*smax_lowres %d priv_class %p profiles %p\n", indent, "",
		codec->max_lowres, codec->priv_class, codec->profiles);
}

void FileAVlibs::dump_AVCodecDescriptor(const AVCodecDescriptor *avdsc, int indent)
{
	const char *s;

	printf("%*sAVCodecDescriptor %p dump:\n", indent, "", avdsc);
	indent += 2;
	printf("%*sid %d type %s name '%s'\n", indent, "", avdsc->id,
		dump_AVMediaType(avdsc->type), avdsc->name);
	printf("%*slong_name '%s'\n", indent, "", avdsc->long_name);
}

void FileAVlibs::dump_AVOutputFormat(const AVOutputFormat *ofmt, int indent)
{
	printf("%*sAVOutputFormat %p dump:\n", indent, "", ofmt);
	indent += 2;
	printf("%*sname: '%s' mime_type: '%s'\n", indent, "", ofmt->name, ofmt->mime_type);
	printf("%*slong_name '%s'\n", indent, "", ofmt->long_name);
	printf("%*saudio_codec '%s' video_codec '%s' subtitle_codec '%s' data_codec '%s'\n", indent, "",
		avcodec_get_name(ofmt->audio_codec), avcodec_get_name(ofmt->video_codec),
		avcodec_get_name(ofmt->subtitle_codec), avcodec_get_name(ofmt->data_codec));
	printf("%*sflags %#x codec_tag %p priv_class %p\n", indent, "",
		ofmt->flags, ofmt->codec_tag, ofmt->priv_class);
}

void FileAVlibs::dump_AVInputFormat(const AVInputFormat *ifmt, int indent)
{
	printf("%*sAVInputFormat %p dump:\n", indent, "", ifmt);
	indent += 2;
	printf("%*sname: '%s' mime_type: '%s'\n", indent, "", ifmt->name, ifmt->mime_type);
	printf("%*slong_name '%s'\n", indent, "", ifmt->long_name);
	printf("%*sflags %#x extensions '%s' codec_tag %p priv_class %p\n", indent, "",
		ifmt->flags, ifmt->extensions, ifmt->codec_tag, ifmt->priv_class);
}

void FileAVlibs::dump_AVOption(const AVOption *opt, const AVClass *avclass, int indent)
{
	printf("%*sAVOption %p dump:\n", indent, "", opt);
	indent += 2;
	printf("%*sname '%s' unit '%s'\n", indent, "", opt->name, opt->unit);
	printf("%*shelp '%s'\n", indent, "", opt->help);
	printf("%*soffset %d type '%s' flags '%s'",
		indent, "",
		opt->offset, dump_AVOptionType(opt->type),
		dump_AVOptionFlags(opt->flags));
	switch(opt->type)
	{
	case AV_OPT_TYPE_INT:
		if(opt->min != INT_MIN)
			printf(" min %.0f", opt->min);
		if(opt->max != INT_MAX)
			printf(" max %.0f", opt->max);
		break;
	case AV_OPT_TYPE_INT64:
		if(opt->min != INT64_MIN)
			printf(" min %.0f", opt->min);
		if(opt->max != INT64_MAX)
			printf(" max %.0f", opt->max);
		break;
	case AV_OPT_TYPE_FLOAT:
		if(opt->min != -FLT_MAX)
			printf(" min %.3f", opt->min);
		if(opt->max != FLT_MAX)
			printf(" max %.3f", opt->max);
		break;
	case AV_OPT_TYPE_DOUBLE:
		if(opt->min != -DBL_MAX)
			printf(" min %.3f", opt->min);
		if(opt->max != DBL_MAX)
			printf(" max %.3f", opt->max);
		break;
	}
	fputc('\n', stdout);
	switch(opt->type)
	{
	case AV_OPT_TYPE_CONST:
	case AV_OPT_TYPE_FLAGS:
		printf("%*sdefault %#" PRIx64 "\n", indent, "", opt->default_val.i64);
		break;
	case AV_OPT_TYPE_INT:
	case AV_OPT_TYPE_INT64:
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(55, 17, 103)
	case AV_OPT_TYPE_BOOL:
#endif
		printf("%*sdefault %" PRId64 "\n", indent, "", opt->default_val.i64);
		break;
	case AV_OPT_TYPE_DOUBLE:
	case AV_OPT_TYPE_FLOAT:
		printf("%*sdefault %#.3f\n", indent, "", opt->default_val.dbl);
		break;
	case AV_OPT_TYPE_STRING:
		printf("%*sdefault '%s'\n", indent, "", opt->default_val.str);
		break;
	}
	if(opt->unit && avclass)
	{
		const AVOption *u = 0;

		printf("%*sPossible values:\n", indent, "");
		while(u = av_opt_next(&avclass, u))
		{
			if(u->type == AV_OPT_TYPE_CONST && u->unit &&
					!strcmp(u->unit, opt->unit))
				dump_AVOption(u, 0, indent + 2);
		}
	}
}

void FileAVlibs::dump_AVOptions(const AVClass *avclass, int indent)
{
	const AVOption *opt = 0;

	printf("%*sAVOptions %p dump:\n", indent, "", avclass);

	while(opt = av_opt_next(&avclass, opt))
	{
		if(opt->type != AV_OPT_TYPE_CONST)
			dump_AVOption(opt, avclass, indent + 2);
	}
}

const char *FileAVlibs::dump_AVOptionType(enum AVOptionType type)
{
	switch(type)
	{
	case AV_OPT_TYPE_FLAGS:
		return "flags";
	case AV_OPT_TYPE_INT:
		return "int";
	case AV_OPT_TYPE_INT64:
		return "int64";
	case AV_OPT_TYPE_DOUBLE:
		return "double";
	case AV_OPT_TYPE_FLOAT:
		return "float";
	case AV_OPT_TYPE_STRING:
		return "string";
	case AV_OPT_TYPE_RATIONAL:
		return "rational";
	case AV_OPT_TYPE_BINARY:
		return "binary";
	case AV_OPT_TYPE_DICT:
		return "dict";
	case AV_OPT_TYPE_CONST:
		return "const";
	case AV_OPT_TYPE_IMAGE_SIZE:
		return "image_size";
	case AV_OPT_TYPE_PIXEL_FMT:
		return "pixel_fmt";
	case AV_OPT_TYPE_SAMPLE_FMT:
		return "sample_fmt";
	case AV_OPT_TYPE_VIDEO_RATE:
		return "video_rate";
	case AV_OPT_TYPE_DURATION:
		return "duration";
	case AV_OPT_TYPE_COLOR:
		return "color";
	case AV_OPT_TYPE_CHANNEL_LAYOUT:
		return "chnl_layout";
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(55, 17, 103)
	case AV_OPT_TYPE_BOOL:
		return "bool";
#endif
	}
	return "Unknown";
}

char *FileAVlibs::dump_AVOptionFlags(int flags)
{
	static char flbuf[64];
	char *bp = flbuf;

	if(flags & AV_OPT_FLAG_ENCODING_PARAM)
		*bp++ = 'E';
	if(flags & AV_OPT_FLAG_DECODING_PARAM)
		*bp++ = 'D';
#if FF_API_OPT_TYPE_METADATA
	if(flags & AV_OPT_FLAG_METADATA)
		*bp++ = 'M';
#endif
	if(flags & AV_OPT_FLAG_AUDIO_PARAM)
		*bp++ = 'A';
	if(flags & AV_OPT_FLAG_VIDEO_PARAM)
		*bp++ = 'V';
	if(flags & AV_OPT_FLAG_SUBTITLE_PARAM)
		*bp++ = 'S';
	if(flags & AV_OPT_FLAG_EXPORT)
		*bp++ = 'X';
	if(flags & AV_OPT_FLAG_READONLY)
		*bp++ = 'R';
	if(flags & AV_OPT_FLAG_FILTERING_PARAM)
		*bp++ = 'F';
	*bp++ = 0;
	return flbuf;
}

void FileAVlibs::dump_AVFormatContext(AVFormatContext *ctx, int indent)
{
	char bf1[256], bf2[64];

	printf("%*sAVFormatContext %p dump:\n", indent, "", ctx);
	indent += 2;
	printf("%*sav_class %p iformat %p oformat %p, priv_data %p\n", indent, "",
		ctx->av_class, ctx->iformat, ctx->oformat, ctx->priv_data);
	printf("%*siocontext %p ctx_flags %#x nb_streams %u streams %p\n", indent, "",
		ctx->pb, ctx->ctx_flags, ctx->nb_streams, ctx->streams);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
	printf("%*sfilename '%s'\n", indent, "", ctx->filename);
#else
	printf("%*surl '%s'\n", indent, "", ctx->url);
#endif
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,100)
	printf("%*sstart_time %s(%.2f) duration %s(%.2f), bit_rate %d\n", indent, "",
#else
	printf("%*sstart_time %s(%.2f) duration %s(%.2f), bit_rate %" PRId64 "\n", indent, "",
#endif
		dump_ts(ctx->start_time, bf1), (double)ctx->start_time / AV_TIME_BASE,
		dump_ts(ctx->duration, bf2), (double)ctx->duration / AV_TIME_BASE, ctx->bit_rate);
	printf("%*spacket_size %u max_delay %d flags '%s'(%#x)\n", indent, "",
		ctx->packet_size, ctx->max_delay, dump_avfmt_flag(ctx->flags, bf1), ctx->flags);
	printf("%*skey %p keylen %d nb_programs %d, programs %p\n", indent, "",
		ctx->key, ctx->keylen, ctx->nb_programs, ctx->programs);
	printf("%*saudio_codec_id %d video_codec_id %d subtitle_codec_id %d data_codec_id %d\n", indent, "",
		ctx->audio_codec_id, ctx->video_codec_id, ctx->subtitle_codec_id, ctx->data_codec_id);
	printf("%*smax_index_size %u max_picture_buffer %u nb_chapters %u chapters %p\n", indent, "",
		ctx->max_index_size, ctx->max_picture_buffer, ctx->nb_chapters, ctx->chapters);
	printf("%*smetadata %p start_time_realtime %s fps_probe_size %d error_recognition %d\n", indent, "",
		ctx->metadata, dump_ts(ctx->start_time_realtime, bf1),
		ctx->fps_probe_size, ctx->error_recognition);
	printf("%*sdebug %d max_interleave_delta %" PRId64 " strict_std_compliance %d\n", indent, "",
		ctx->debug, ctx->max_interleave_delta, ctx->strict_std_compliance);
	printf("%*smax_ts_probe %d avoid_negative_ts %d audio_preload %d max_chunk_duration %d\n", indent, "",
		ctx->max_ts_probe, ctx->avoid_negative_ts, ctx->audio_preload, ctx->max_chunk_duration);
	printf("%*sduration_estimation_method %d skip_initial_bytes %" PRId64 " correct_ts_overflow %u\n", indent, "",
		ctx->duration_estimation_method, ctx->skip_initial_bytes, ctx->correct_ts_overflow);
	printf("%*sseek2any %d flush_packets %d probe_score %d format_probesize %d\n", indent, "",
		ctx->seek2any, ctx->flush_packets, ctx->probe_score, ctx->format_probesize);
	printf("%*scodec_whitelist %p format_whitelist %p internal %p io_repositioned %d\n", indent, "",
		ctx->codec_whitelist, ctx->format_whitelist, ctx->internal, ctx->io_repositioned);
	printf("%*svideo_codec %p audio_codec %p subtitle_codec %p data_codec %p\n", indent, "",
		ctx->video_codec, ctx->audio_codec, ctx->subtitle_codec, ctx->data_codec);
}

const char *FileAVlibs::dump_ts(int64_t ts, char *obuf)
{
	static char lbuf[64];

	if(ts == AV_NOPTS_VALUE)
		return "<nopts>";

	if(!obuf)
		obuf = lbuf;

	sprintf(obuf, "%" PRId64, ts);
	return obuf;
}

const char *FileAVlibs::dump_avfmt_flag(int flags, char *obuf)
{
	int l;

	obuf[0] = 0;

	if(flags & AVFMT_FLAG_GENPTS)
		strcat(obuf, "genpts,");
	if(flags & AVFMT_FLAG_IGNIDX)
		strcat(obuf, "inidx,");
	if(flags & AVFMT_FLAG_NONBLOCK)
		strcat(obuf, "nonblock,");
	if(flags & AVFMT_FLAG_IGNDTS)
		strcat(obuf, "igndts,");
	if(flags & AVFMT_FLAG_NOFILLIN)
		strcat(obuf, "nofillin,");
	if(flags & AVFMT_FLAG_NOPARSE)
		strcat(obuf, "noparse,");
	if(flags & AVFMT_FLAG_NOBUFFER)
		strcat(obuf, "nobuffer,");
	if(flags & AVFMT_FLAG_CUSTOM_IO)
		strcat(obuf, "customio,");
	if(flags & AVFMT_FLAG_DISCARD_CORRUPT)
		strcat(obuf, "discardcrpt,");
	if(flags & AVFMT_FLAG_FLUSH_PACKETS)
		strcat(obuf, "flushpkts,");
	if(flags & AVFMT_FLAG_BITEXACT)
		strcat(obuf, "bitexact,");
	if(flags & AVFMT_FLAG_MP4A_LATM)
		strcat(obuf, "mp4alatm,");
	if(flags & AVFMT_FLAG_SORT_DTS)
		strcat(obuf, "sortdts,");
	if(flags & AVFMT_FLAG_PRIV_OPT)
		strcat(obuf, "privopt,");
	if(flags & AVFMT_FLAG_KEEP_SIDE_DATA)
		strcat(obuf, "keepside,");
	if(flags & AVFMT_FLAG_FAST_SEEK)
		strcat(obuf, "fastseek,");

	if((l = strlen(obuf)) > 0)
		obuf[l -1] = 0;
	return obuf;
}

const char *FileAVlibs::dump_AVRational(const AVRational *r, char *obuf)
{
	char *bp;
	static char lbuf[64];

	if(!obuf)
		obuf = lbuf;
	sprintf(obuf, "%d/%d", r->num, r->den);
	return obuf;
}

void FileAVlibs::dump_AVStream(AVStream *stm, int indent)
{
	char bf1[64], bf2[64];

	printf("%*sAVStream %p dump:\n", indent, "", stm);
	indent += 2;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,41,100)
	printf("%*sindex %d id %d codec %p priv_data %p\n", indent, "",
		stm->index, stm->id, stm->codec, stm->priv_data);
#else
	printf("%*sindex %d id %d codecpar %p priv_data %p\n", indent, "",
		stm->index, stm->id, stm->codecpar, stm->priv_data);
#endif
	printf("%*stime_base %s start_time %s duration %s frames %" PRId64 "\n", indent, "",
		dump_AVRational(&stm->time_base), dump_ts(stm->start_time, bf1),
		dump_ts(stm->duration, bf2), stm->nb_frames);
	printf("%*sdisposition %#x discard %d, sample_aspect_ratio %s\n", indent, "",
		stm->disposition, stm->discard, dump_AVRational(&stm->sample_aspect_ratio));
	printf("%*smetadata %p avg_frame_rate %s side_data %p nb_side_data %d\n", indent, "",
		stm->metadata, dump_AVRational(&stm->avg_frame_rate),
		stm->side_data, stm->nb_side_data);
	printf("%*sattached_pic:\n", indent, "");
	dump_AVPacket(&stm->attached_pic, indent + 2);
	printf("%*sevent_flags %d first_dts %s cur_dts %s\n", indent, "",
		stm->event_flags, dump_ts(stm->first_dts, bf1),
		dump_ts(stm->cur_dts, bf2));
	printf("%*slast_IP_pts %s last_IP_duration %d probe_packets %d codec_info_nb_frames %d\n", indent, "",
		dump_ts(stm->last_IP_pts), stm->last_IP_duration, stm->probe_packets,
		stm->codec_info_nb_frames);
	printf("%*sneed_parsing %d\n", indent, "", stm->need_parsing);
	printf("%*sindex_entries %p nb_index_entries %d\n", indent, "",
		stm->index_entries, stm->nb_index_entries);
	printf("%*sindex_entries_allocated_size %u r_frame_rate %s stream_identifier %d\n", indent, "",
		stm->index_entries_allocated_size, dump_AVRational(&stm->r_frame_rate),
		stm->stream_identifier);
}

const char *FileAVlibs::dump_fourcc(unsigned int tag)
{
	static char bf[6];

	bf[0] = tag & 0xff;
	bf[1] = (tag >> 8) & 0xff;
	bf[2] = (tag >> 16) & 0xff;
	bf[3] = (tag >> 24) & 0xff;
	bf[4] = 0;

	return bf;
}

void FileAVlibs::dump_AVCodecContext(AVCodecContext *ctx, int indent)
{
	char bf1[64], bf2[64];

	printf("%*sAVCodecContext %p dump:\n", indent, "", ctx);
	indent += 2;
	printf("%*sav_class %p log_level_offset %d codec_type %d codec %p\n", indent, "",
		ctx->av_class, ctx->log_level_offset, ctx->codec_type, ctx->codec);
	printf("%*scodec_id '%s' fourcc '%s' priv_data %p, internal %p\n", indent, "",
		avcodec_get_name(ctx->codec_id), dump_fourcc(ctx->codec_tag),
		ctx->priv_data, ctx->internal);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,100)
	printf("%*sopaque %p bit_rate %d bit_rate_tolerance %d global_quality %d\n", indent, "",
#else
	printf("%*sopaque %p bit_rate %" PRId64 " bit_rate_tolerance %d global_quality %d\n", indent, "",
#endif
		ctx->opaque, ctx->bit_rate, ctx->bit_rate_tolerance, ctx->global_quality);
	printf("%*scompression_level %d flags %#x flags2 %d extradata %p extradata_size %d\n", indent, "",
		ctx->compression_level, ctx->flags, ctx->flags2, ctx->extradata,
		ctx->extradata_size);
	printf("%*stime_base %s ticks_per_frame %d delay %d w/h [%d,%d] coded [%d,%d]\n", indent, "",
		dump_AVRational(&ctx->time_base), ctx->ticks_per_frame, ctx->delay,
		ctx->width, ctx->height, ctx->coded_width, ctx->coded_height);
	printf("%*sgop_size %d, pix_fmt '%s' draw_horiz_band %p get_format %p\n", indent, "",
		ctx->gop_size, av_get_pix_fmt_name(ctx->pix_fmt),
		ctx->draw_horiz_band, ctx->get_format);
	printf("%*smax_b_frames %d b_quant_factor %.2f b_quant_offset %.2f\n", indent, "",
		ctx->max_b_frames, ctx->b_quant_factor, ctx->b_quant_offset);
	printf("%*shas_b_frames %d i_quant_factor %.2f i_quant_offset %.2f\n", indent, "",
		ctx->has_b_frames, ctx->i_quant_factor, ctx->i_quant_offset);
	printf("%*slumi_masking %.2f temporal_cplx_masking %.2f spatial_cplx_masking %.2f\n", indent, "",
		ctx->lumi_masking, ctx->temporal_cplx_masking, ctx->spatial_cplx_masking);
	printf("%*sp_masking %.2f dark_masking %.2f slice_count %d\n", indent, "",
		ctx->p_masking, ctx->dark_masking, ctx->slice_count);
	printf("%*sslice_offset %p sample_aspect_ratio %s me_cmp %d me_sub_cmp %d\n", indent, "",
		ctx->slice_offset, dump_AVRational(&ctx->sample_aspect_ratio),
		ctx->me_cmp, ctx->me_sub_cmp);
	printf("%*smb_cmp %d ildct_cmp %d dia_size %d last_predictor_count %d\n", indent, "",
		ctx->mb_cmp, ctx->ildct_cmp, ctx->dia_size, ctx->last_predictor_count);
	printf("%*sme_pre_cmp %d pre_dia_size %d me_subpel_quality %d me_range %d\n", indent, "",
		ctx->me_pre_cmp, ctx->pre_dia_size, ctx->me_subpel_quality, ctx->me_range);
	printf("%*sslice_flags %d mb_decision %d intra_matrix %p inter_matrix %p\n", indent, "",
		ctx->slice_flags, ctx->mb_decision, ctx->intra_matrix, ctx->inter_matrix);
	printf("%*sintra_dc_precision %d skip_top %d skip_bottom %d mb_lmin %d mb_lmax %d\n", indent, "",
		ctx->intra_dc_precision, ctx->skip_top, ctx->skip_bottom,
		ctx->mb_lmin, ctx->mb_lmax);
	printf("%*sbidir_refine %d keyint_min %d refs %d mv0_threshold %d\n", indent, "",
		ctx->mv0_threshold, ctx->bidir_refine, ctx->keyint_min, ctx->refs);
	printf("%*scolor_primaries '%s' color_trc '%s'\n", indent, "",
		av_color_primaries_name(ctx->color_primaries),
		av_color_transfer_name(ctx->color_trc));
	printf("%*scolorspace '%s' color_range '%s' chroma_sample_location '%s' \n", indent, "",
		av_color_space_name(ctx->colorspace),
		av_color_range_name(ctx->color_range),
		av_chroma_location_name(ctx->chroma_sample_location));
	printf("%*sslices %d field_order %d sample_rate %d channels %d, sample_fmt '%s'\n", indent, "",
		ctx->slices, ctx->field_order, ctx->sample_rate, ctx->channels,
		av_get_sample_fmt_name(ctx->sample_fmt));
	printf("%*sframe_size %d frame_number %d block_align %d cutoff %d\n", indent, "",
		ctx->frame_size, ctx->frame_number, ctx->block_align, ctx->cutoff);
	printf("%*schannel_layout %#" PRIx64 " request_channel_layout %#" PRIx64 " audio_service_type %d\n", indent, "",
		ctx->channel_layout, ctx->request_channel_layout, ctx->audio_service_type);
	printf("%*srequest_sample_fmt %d get_buffer2 %p\n", indent, "",
		ctx->request_sample_fmt, ctx->get_buffer2);
	printf("%*sqcompress %.2f qblur %.2f qmin %d qmax %d max_qdiff %d\n", indent, "",
		ctx->qcompress, ctx->qblur, ctx->qmin, ctx->qmax, ctx->max_qdiff);
	printf("%*src_buffer_size %d rc_override_count %d rc_override %p\n", indent, "",
		ctx->rc_buffer_size, ctx->rc_override_count, ctx->rc_override);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,100)
	printf("%*src_max_rate %d rc_min_rate %d rc_max_available_vbv_use %.2f\n", indent, "",
#else
	printf("%*src_max_rate %" PRId64 " rc_min_rate %" PRId64 " rc_max_available_vbv_use %.2f\n", indent, "",
#endif
		ctx->rc_max_rate, ctx->rc_min_rate, ctx->rc_max_available_vbv_use);
	printf("%*src_min_vbv_overflow_use %.2f rc_initial_buffer_occupancy %d\n", indent, "",
		ctx->rc_min_vbv_overflow_use, ctx->rc_initial_buffer_occupancy);
	printf("%*strellis %d stats_out %p stats_in %p, workaround_bugs %#x\n", indent, "",
		ctx->trellis, ctx->stats_out, ctx->stats_in, ctx->workaround_bugs);
	printf("%*sstrict_std_compliance %d error_concealment %d debug %#x\n", indent, "",
		ctx->strict_std_compliance, ctx->error_concealment, ctx->debug);
	printf("%*serr_recognition %#x reordered_opaque %s hwaccel %p hwaccel_context %p\n", indent, "",
		ctx->err_recognition, dump_ts(ctx->reordered_opaque), ctx->hwaccel,
		ctx->hwaccel_context);
	printf("%*sdct_algo %d idct_algo %d bits_per_coded_sample %d bits_per_raw_sample %d\n", indent, "",
		ctx->dct_algo, ctx->idct_algo, ctx->bits_per_coded_sample, ctx->bits_per_raw_sample);
	printf("%*sthread_count %d thread_type %d active_thread_type %d\n", indent, "",
		ctx->thread_count, ctx->thread_type, ctx->active_thread_type);
	printf("%*sexecute %p execute2 %p nsse_weight %d profile %d level %d\n", indent, "",
		ctx->execute, ctx->execute2, ctx->nsse_weight, ctx->profile, ctx->level);
	printf("%*sskip_loop_filter %d skip_idct %d skip_frame %d\n", indent, "",
		ctx->skip_loop_filter, ctx->skip_idct, ctx->skip_frame);
	printf("%*ssubtitle_header %p subtitle_header_size %d\n", indent, "",
		ctx->subtitle_header, ctx->subtitle_header_size);
	printf("%*sinitial_padding %d framerate %s\n", indent, "",
		ctx->initial_padding, dump_AVRational(&ctx->framerate));
	printf("%*ssw_pix_fmt %d pkt_timebase %s codec_descriptor %p\n", indent, "",
		ctx->sw_pix_fmt, dump_AVRational(&ctx->pkt_timebase), ctx->codec_descriptor);
	printf("%*spts_correction_num_faulty_pts %" PRId64 " pts_correction_num_faulty_dts %" PRId64 "\n", indent, "",
		ctx->pts_correction_num_faulty_pts, ctx->pts_correction_num_faulty_dts);
	printf("%*spts_correction_last_pts %s pts_correction_last_dts %s\n", indent, "",
		dump_ts(ctx->pts_correction_last_pts, bf1),
		dump_ts(ctx->pts_correction_last_dts, bf2));
	printf("%*ssub_charenc %p sub_charenc_mode %d skip_alpha %d seek_preroll %d\n", indent, "",
		ctx->sub_charenc, ctx->sub_charenc_mode, ctx->skip_alpha, ctx->seek_preroll);
	printf("%*sdump_separator %p codec_whitelist %p properties %u\n", indent, "",
		ctx->dump_separator, ctx->codec_whitelist, ctx->properties);
}

void FileAVlibs::dump_AVFrame(AVFrame *avf, int indent)
{
	char bf1[64], bf2[64];
	const char *s;

	printf("%*sAVFrame %p dump:\n", indent, "", avf);
	indent += 2;
	printf("%*sdata: ", indent, "");
	for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
	{
		printf(" %p", avf->data[i]);
		if(i < AV_NUM_DATA_POINTERS - 1)
			putchar(',');
	}
	putchar('\n');

	printf("%*slinesize: ", indent, "");
	for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
	{
		printf(" %d", avf->linesize[i]);
		if(i < AV_NUM_DATA_POINTERS - 1)
			putchar(',');
	}
	putchar('\n');

	printf("%*sbuf: ", indent, "");
	for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
	{
		printf(" %p", avf->buf[i]);
		if(i < AV_NUM_DATA_POINTERS - 1)
			putchar(',');
	}
	putchar('\n');

	printf("%*sextended_data %p [%d,%d] nb_samples %d format ", indent, "",
		avf->extended_data, avf->width, avf->height, avf->nb_samples);
	if(avf->height)
		s = av_get_pix_fmt_name((AVPixelFormat)avf->format);
	else if(avf->sample_rate)
		s = av_get_sample_fmt_name((AVSampleFormat)avf->format);
	else
		s = 0;
	if(s)
		printf(" '%s'\n", s);
	else
		printf(" %d\n", avf->format);
	printf("%*skey_frame %d pict_type %d sample_aspect_ratio %s pts %s\n", indent, "",
		avf->key_frame, avf->pict_type, dump_AVRational(&avf->sample_aspect_ratio),
		dump_ts(avf->pts));
	printf("%*spkt_dts %s coded_picture_number %d display_picture_number %d\n", indent, "",
		dump_ts(avf->pkt_dts, bf2),
		avf->coded_picture_number, avf->display_picture_number);
	printf("%*squality %d opaque %p repeat_pict %d interlaced_frame %d top_field_first %d\n", indent, "",
		avf->quality, avf->opaque, avf->repeat_pict, avf->interlaced_frame,
		avf->top_field_first);
	printf("%*spalette_has_changed %d reordered_opaque %s sample_rate %d\n", indent, "",
		avf->palette_has_changed, dump_ts(avf->reordered_opaque), avf->sample_rate);
	printf("%*schannel_layout %#" PRIx64 " extended_buf %p nb_extended_buf %d\n", indent, "",
		avf->channel_layout, avf->extended_buf, avf->nb_extended_buf);
	printf("%*sside_data %p nb_side_data %d flags %d best_effort_timestamp %s\n", indent, "",
		avf->side_data, avf->nb_side_data, avf->flags,
		dump_ts(avf->best_effort_timestamp));
	printf("%*scolor_range '%s' color_primaries '%s'\n", indent, "",
		av_color_range_name(avf->color_range),
		av_color_primaries_name(avf->color_primaries));
	printf("%*scolor_trc '%s' colorspace '%s' chroma_location '%s'\n", indent, "",
		av_color_transfer_name(avf->color_trc), av_color_space_name(avf->colorspace),
		av_chroma_location_name(avf->chroma_location));
	printf("%*spkt_pos %s pkt_duration %s metadata %p decode_error_flags %d\n", indent, "",
		dump_ts(avf->pkt_pos, bf1), dump_ts(avf->pkt_duration, bf2),
		avf->metadata, avf->decode_error_flags);
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
	printf("%*schannels %d pkt_size %d qp_table_buf %p\n", indent, "",
		avf->channels, avf->pkt_size, avf->qp_table_buf);
#else
	printf("%*schannels %d pkt_size %d\n", indent, "",
		avf->channels, avf->pkt_size);
#endif
}

void FileAVlibs::dump_AVPacket(AVPacket *pkt, int indent)
{
	char bf1[64], bf2[64];

	printf("%*sAVPacket %p dump:\n", indent, "", pkt);
	indent += 2;
	printf("%*sbuf %p pts %s dts %s data %p size %d stream_index %d flags %#x\n", indent, "",
		pkt->buf, dump_ts(pkt->pts, bf1), dump_ts(pkt->dts, bf2),
		pkt->data, pkt->size, pkt->stream_index, pkt->flags);
	printf("%*sside_data %p side_data_elems %d duration %s pos %" PRId64 "\n", indent, "",
		pkt->side_data, pkt->side_data_elems, dump_ts(pkt->duration), pkt->pos);
}

void FileAVlibs::dump_AVProgram(AVProgram *prg, int indent)
{
	char bf1[64], bf2[64], bf3[64];

	printf("%*sAVProgram %p dump:\n", indent, "", prg);
	indent += 2;
	printf("%*sid %d flags %#x discard %d stream indexes %d metadata %p\n", indent, "",
		prg->id, prg->flags, prg->discard, prg->nb_stream_indexes, prg->metadata);
	if(prg->nb_stream_indexes > 0)
	{
		printf("%*sstream indexes:", indent, "");
		for(int i = 0; i < prg->nb_stream_indexes; i++)
			printf(" %d", prg->stream_index[i]);
		putchar('\n');
	}
	printf("%*sstart %s end %s wrapref %s w_behav %d\n", indent, "",
		dump_ts(prg->start_time, bf1), dump_ts(prg->end_time, bf2),
		dump_ts(prg->pts_wrap_reference, bf3), prg->pts_wrap_behavior);
}

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57,41,100)
void FileAVlibs::dump_AVCodecParameters(AVCodecParameters *codecpar, int indent)
{
	const char *s;

	printf("%*sAVCodecParameters %p dump:\n", indent, "", codecpar);
	indent += 2;
	printf("%*scodec_type '%s' codec_id '%s' codec_tag '%s'\n", indent, "",
		av_get_media_type_string(codecpar->codec_type),
		avcodec_get_name(codecpar->codec_id), dump_fourcc(codecpar->codec_tag));
	printf("%*sextradata %p extradata_size %d format ", indent, "",
		codecpar->extradata, codecpar->extradata_size);

	switch(codecpar->codec_type)
	{
	case AVMEDIA_TYPE_VIDEO:
		s = av_get_pix_fmt_name((AVPixelFormat)codecpar->format);
		break;
	case AVMEDIA_TYPE_AUDIO:
		s = av_get_sample_fmt_name((AVSampleFormat)codecpar->format);
		break;
	default:
		s = 0;
		break;
	}
	if(s)
		printf(" '%s'\n", s);
	else
		printf(" %d\n", codecpar->format);

	printf("%*sbit_rate %" PRId64 " bits_per_coded_sample %d bits_per_raw_sample %d\n", indent, "",
		codecpar->bit_rate, codecpar->bits_per_coded_sample, codecpar->bits_per_raw_sample);
	printf("%*sprofile %d level %d size [%dx%d] sample_aspect_ratio %s\n", indent, "",
		codecpar->profile, codecpar->level, codecpar->width, codecpar->height,
		dump_AVRational(&codecpar->sample_aspect_ratio));
	printf("%*sfield_order %d color_range '%s' color_primaries '%s' color_trc '%s'\n", indent, "",
		codecpar->field_order, av_color_range_name(codecpar->color_range),
		av_color_primaries_name(codecpar->color_primaries),
		av_color_transfer_name(codecpar->color_trc));
	printf("%*schroma_location '%s' video_delay %d channel_layout %" PRIx64 "\n", indent, "",
		av_chroma_location_name(codecpar->chroma_location),
		codecpar->video_delay, codecpar->channel_layout);
	printf("%*schannels %d sample_rate %d block_align %d frame_size %d\n", indent, "",
		codecpar->channels, codecpar->sample_rate, codecpar->block_align, codecpar->frame_size);
	printf("%*sinitial_padding %d trailing_padding %d seek_preroll %d\n", indent, "",
		codecpar->initial_padding, codecpar->trailing_padding, codecpar->seek_preroll);
}
#endif
