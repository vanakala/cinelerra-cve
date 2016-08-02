
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

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
#include "byteorder.h"
#include "clip.h"
#include "file.h"
#include "fileavlibs.h"
#include "filetoc.h"
#include "mainerror.h"
#include "mutex.h"
#include "paramlist.h"
#include "preferences.h"
#include "selection.h"
#include "theme.h"
#include "videodevice.inc"
#include "vframe.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

// Current codecs list in asset
#define PARAM_CODEC_AUDIO "audio"
#define PARAM_CODEC_VIDEO "video"

extern Theme *theme_global;

struct  avlib_formattable FileAVlibs::known_formats[] =
{
	{ FILE_MOV, "mov,mp4,m4a,3gp,3g2,mj2", "mov" },
	{ FILE_AVI, "avi", "avi"  },
	{ 0 }
};

extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavformat/internal.h>
}

Mutex* FileAVlibs::avlibs_lock = new Mutex("FileAVlibs::avlibs_lock");

FileAVlibs::FileAVlibs(Asset *asset, File *file)
 : FileBase(asset, file)
{
	avvframe = 0;
	avaframe = 0;
	audio_index = -1;
	video_index = -1;
	audio_eof = 0;
	context = 0;
	sws_ctx = 0;
	swr_ctx = 0;
	temp_frame = 0;
	tocfile = 0;
	temp_frame = 0;
	av_log_set_level(AV_LOG_QUIET);
	memset(&track_data, 0, sizeof(track_data));
	num_buffers = 0;
	buffer_len = 0;
	memset(abuffer, 0, sizeof(abuffer));
	resampled_alloc = 0;
	memset(resampled_data, 0, sizeof(resampled_data));
}

FileAVlibs::~FileAVlibs()
{
	for(int i = 0; i < MAXCHANNELS; i++)
		delete [] abuffer[i];
	for(int i = 0; i < MAXCHANNELS; i++)
		delete [] resampled_data[i];
	close_file();
}

int FileAVlibs::check_sig(Asset *asset)
{
	AVFormatContext *ctx = 0;
	int result = 0;

	avlibs_lock->lock("FileAVlibs::check_sig");
	avcodec_register_all();
	av_register_all();

	if(avformat_open_input(&ctx, asset->path, 0, NULL) == 0)
	{
		if(avformat_find_stream_info(ctx, NULL) >= 0)
			result = 1;

		if(result && streamformat(ctx) == FILE_UNKNOWN)
			result = 0;

		avformat_close_input(&ctx);
	}
	avlibs_lock->unlock();

	return result;
}

int FileAVlibs::open_file(int rd, int wr)
{
	int result = 0;
	int rv;

	avlibs_lock->lock("FileAVlibs::open_file");
	avcodec_register_all();
	av_register_all();

	reading = rd;
	writing = wr;
	audio_pos = 0;
	video_pos = 0;
	pts_base = -1;
	headers_written = 0;

	if(rd)
	{
		struct stat stbuf;
		if(stat(asset->path, &stbuf) < 0)
		{
			errorbox("FileAVlibs::open_file:Failed to stat %s",
				asset->path);
			return 1;
		}
		if(!S_ISREG(stbuf.st_mode))
		{
			errorbox("FileAVlibs::open_file::%s is not a regular file",
				asset->path);
			return 1;
		}
		if(avformat_open_input(&context, asset->path, 0, NULL) == 0)
			result = avformat_find_stream_info(context, NULL);
		else
		{
			avlibs_lock->unlock();
			return 1;
		}
		asset->format = streamformat(context);

		if(result >= 0)
		{
			result = 0;
			asset->file_length = stbuf.st_size;

			for(int i = 0; i < context->nb_streams; i++)
			{
				AVStream *stream = context->streams[i];
				AVCodecContext *decoder_context = stream->codec;
				switch(decoder_context->codec_type)
				{
				case AVMEDIA_TYPE_AUDIO:
					if(audio_index < 0)
					{
						audio_index = i;
						asset->audio_data = 1;
						asset->channels = decoder_context->channels;
						asset->sample_rate = decoder_context->sample_rate;

						AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
						if(!codec)
						{
							errormsg("FileAVLibs::open_file: audio codec %#x not found",
								decoder_context->codec_id);
							asset->audio_data = 0;
							audio_index = -1;
						}
						else
						{
							if((rv = avcodec_open2(decoder_context, codec, NULL)) < 0)
							{
								liberror(rv, "FileAVLibs::open_file: Failed to open audio decoder");
								asset->audio_data = 0;
								audio_index = -1;
							}
							else
							{
								strncpy(asset->acodec, codec->name, 4);
								asset->bits = av_get_bytes_per_sample(decoder_context->sample_fmt) * 8;
								audio_delay = 0;
								buffer_start = buffer_end = 0;
							}
						}
					}
					break;

				case AVMEDIA_TYPE_VIDEO:
					if(video_index < 0)
					{
						video_index = i;
						asset->video_data = 1;
						asset->layers = 1;
						asset->width = decoder_context->width;
						asset->height = decoder_context->height;

						asset->frame_rate = av_q2d(stream->r_frame_rate);
						asset->aspect_ratio =
								(double)(decoder_context->coded_width * decoder_context->sample_aspect_ratio.num) /
								(decoder_context->coded_height * decoder_context->sample_aspect_ratio.den);
						AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
						if(!codec)
						{
							errormsg("FileAVLibs::open_file: video codec %#x not found",
								decoder_context->codec_id);
							asset->video_data = 0;
							video_index = -1;
						}
						else
						{
							if((rv = avcodec_open2(decoder_context, codec, NULL)) < 0)
							{
								liberror(rv, "FileAVLibs::open_file: Failed to open video decoder");
								asset->video_data = 0;
								video_index = -1;
							}
							else
								strncpy(asset->vcodec, codec->name, 4);
						}
					}
					break;

				default:
					break;
				}
			}
			tocfile = new FileTOC(this, file->preferences->index_directory,
				asset->path, asset->file_length, stbuf.st_mtime);
			result = tocfile->init_tocfile(TOCFILE_TYPE_MUX0);

			if(video_index >= 0)
			{
				asset->streams[video_index].options = STRDSC_VIDEO;
				asset->streams[video_index].start = (ptstime)tocfile->toc_streams[video_index].min_index *
					av_q2d(context->streams[video_index]->time_base);
				asset->streams[video_index].end = (ptstime)tocfile->toc_streams[video_index].max_index *
					av_q2d(context->streams[video_index]->time_base);
				asset->video_length = av_rescale_q(tocfile->toc_streams[video_index].max_index -
						tocfile->toc_streams[video_index].min_index,
						context->streams[video_index]->time_base,
						av_inv_q(context->streams[video_index]->r_frame_rate));
			}
			if(audio_index >= 0)
			{
				asset->streams[audio_index].options = STRDSC_AUDIO;
				asset->streams[audio_index].start = (ptstime)tocfile->toc_streams[audio_index].min_index *
					av_q2d(context->streams[audio_index]->time_base);
				asset->streams[audio_index].end = (ptstime)tocfile->toc_streams[audio_index].max_index *
					av_q2d(context->streams[audio_index]->time_base);
				asset->audio_length = (samplenum)round((tocfile->toc_streams[audio_index].max_index -
					tocfile->toc_streams[audio_index].min_index) *
					av_q2d(context->streams[audio_index]->time_base) *
					asset->sample_rate);
			}
			for(int i = 0; i < MAXCHANNELS; i++)
			{
				if(asset->streams[i].options)
				{
					pts_base = asset->streams[i].start;
					break;
				}
			}
			for(int i = 0; i < MAXCHANNELS; i++)
				if(asset->streams[i].options && pts_base > asset->streams[i].start)
					pts_base = asset->streams[i].start;
		}
		else
		{
			avlibs_lock->unlock();
			return 1;
		}
	}
	else if(wr)
	{
		AVOutputFormat *fmt;
		int rv;
		Param *aparam;

		switch(asset->format)
		{
		case FILE_MOV:
			break;

		default:
			errormsg("FileAVlibs::open_file:Unsupported file type");
			avlibs_lock->unlock();
			return 1;
		}
		if((rv = avformat_alloc_output_context2(&context, NULL, NULL, asset->path)) < 0)
		{
			errormsg("FileAVlibs::open_file:Failed to allocate output context %d", rv);
			avlibs_lock->unlock();
			return 1;
		}

		fmt = context->oformat;

		if(asset->video_data)
		{
			AVCodec *codec;
			AVCodecContext *video_ctx;
			AVStream *stream;
			AVDictionary *dict = create_dictionary(SUPPORTS_VIDEO);

			if(asset->encoder_parameters[FILEAVLIBS_CODECS_IX] &&
				(aparam = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(PARAM_CODEC_VIDEO)))
			{
				if(!(codec = avcodec_find_encoder((AVCodecID)aparam->intvalue)))
				{
					errormsg("FileAVlibs::open_file: Could not find video codec '%s'",
						aparam->stringvalue);
					avlibs_lock->unlock();
					return 1;
				}

				if(!(stream = avformat_new_stream(context, codec)))
				{
					errormsg("FileAVlibs::open_file: Could not allocate video stream");
					avlibs_lock->unlock();
					return 1;
				}
			}
			else
			{
				errormsg("FileAVlibs::open_file:missing video codec");
				avlibs_lock->unlock();
				return 1;
			}

			video_ctx = stream->codec;
			video_index = context->nb_streams - 1;
			video_ctx->width = asset->width;
			video_ctx->height = asset->height;
			video_ctx->time_base = av_d2q(1. / asset->frame_rate, 10000);
			stream->time_base = video_ctx->time_base;
			video_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			video_ctx->sample_aspect_ratio =
				av_mul_q(av_d2q(asset->aspect_ratio, 40),
				(AVRational){video_ctx->height, video_ctx->width});
			// Some formats want stream headers to be separate.
			if(context->oformat->flags & AVFMT_GLOBALHEADER)
				video_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			if((rv = avcodec_open2(video_ctx, codec, &dict)) < 0)
			{
				av_dict_free(&dict);
				liberror(rv, "FileAVlibs::open_file:Could not open video codec");
				avlibs_lock->unlock();
				return 1;
			}
			av_dict_free(&dict);
			if(!(avvframe = av_frame_alloc()))
			{
				errormsg("FileAVlibs::open_file:Could not create video_frame");
				avlibs_lock->unlock();
				return 1;
			}
			avvframe->format = video_ctx->pix_fmt;
			avvframe->width = video_ctx->width;
			avvframe->height = video_ctx->height;
		}

		if(asset->audio_data)
		{
			AVCodec *codec;
			AVCodecContext *audio_ctx;
			AVStream *stream;
			AVDictionary *dict = create_dictionary(SUPPORTS_AUDIO);

			if(asset->encoder_parameters[FILEAVLIBS_CODECS_IX] &&
				(aparam = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(PARAM_CODEC_AUDIO)))
			{
				if(!(codec = avcodec_find_encoder((AVCodecID)aparam->intvalue)))
				{
					errormsg("FileAVlibs::open_file: Could not find audio codec '%s'",
						aparam->stringvalue);
					avlibs_lock->unlock();
					return 1;
				}

				if(!(stream = avformat_new_stream(context, codec)))
				{
					errormsg("FileAVlibs::open_file:Could not allocate allocate audio stream");
					avlibs_lock->unlock();
					return 1;
				}
			}
			else
			{
				errormsg("FileAVlibs::open_file:missing audio codec");
				avlibs_lock->unlock();
				return 1;
			}

			audio_ctx = stream->codec;
			audio_index = context->nb_streams - 1;
			audio_ctx->bit_rate = 64000;
			audio_ctx->sample_rate = asset->sample_rate;
			if(codec->supported_samplerates)
			{
				int i;
				for(i = 0; codec->supported_samplerates[i]; i++)
					if(codec->supported_samplerates[i] == audio_ctx->sample_rate)
						break;
				if(!codec->supported_samplerates[i])
				{
					errormsg("FileAVlibs::open_file:Samplerate %d is not supported by audio codec",
						audio_ctx->sample_rate);
					avlibs_lock->unlock();
					return 1;
				}
			}
			audio_ctx->channels = asset->channels;
			// FIXIT - codec supported layouts must be checked
			audio_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
			audio_ctx->sample_fmt = codec->sample_fmts[0];
			stream->time_base = (AVRational){1, audio_ctx->sample_rate};
			audio_ctx->time_base = stream->time_base;

			// Allow the use of the experimental AAC encoder
			audio_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

			if(context->oformat->flags & AVFMT_GLOBALHEADER)
				audio_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			if((rv = avcodec_open2(audio_ctx, codec, &dict)) < 0)
			{
				av_dict_free(&dict);
				liberror(rv, _("FileAVlibs::open_file:Could not open audio codec"));
				avlibs_lock->unlock();
				return 1;
			}
			av_dict_free(&dict);
			if(!(swr_ctx = swr_alloc_set_opts(NULL,
				audio_ctx->channel_layout,
				audio_ctx->sample_fmt,
				audio_ctx->sample_rate,
				AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT,
				AV_SAMPLE_FMT_DBLP,
				asset->sample_rate, 0, 0)))
			{
				errormsg(_("FileAVlibs::open_file:Can't allocate resample context"));
				avlibs_lock->unlock();
				return 1;
			}

			if(swr_init(swr_ctx))
			{
				errormsg(_("FileAVlibs::open_file: Failed to initalize resample context"));
				avlibs_lock->unlock();
				return 1;
			}

			if(!(avaframe = av_frame_alloc()))
			{
				errormsg(_("FileAVlibs::open_file: Could not allocate audio frame"));
				avlibs_lock->unlock();
				return 1;
			}
		}

		if(video_index < 0 && audio_index < 0)
		{
			avlibs_lock->unlock();
			return 1;
		}

		if(!(fmt->flags & AVFMT_NOFILE))
		{
			if((rv = avio_open(&context->pb, context->filename, AVIO_FLAG_WRITE)) < 0)
			{
				errormsg("FileAVlibs::open_file:Could not open '%s': %d",
					context->filename, rv);
				avlibs_lock->unlock();
				return 1;
			}
		}
		if((rv = avformat_write_header(context, 0)) < 0)
		{
			errormsg("FileAVlibs::open_file:Failed to write header: %d", rv);
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
	avlibs_lock->lock("FileAVlibs:close_file");


	if(context)
	{
		if(reading)
		{
			if(video_index >= 0)
			{
				AVStream *stream = context->streams[video_index];
				if(stream)
				{
					AVCodecContext *decoder_context = stream->codec;
					if(decoder_context) avcodec_close(decoder_context);
				}
				sws_freeContext(sws_ctx);
			}

			if(audio_index >= 0)
			{
				AVStream *stream = context->streams[audio_index];
				if(stream)
				{
					AVCodecContext *decoder_context = stream->codec;
					if(decoder_context) avcodec_close(decoder_context);
				}
			}

			avformat_close_input(&context);
			delete tocfile;
			tocfile = 0;
		}
		else if(writing)
		{
			if(avaframe && headers_written)
			{
				AVPacket pkt;
				int got_output, rv;
				int resampled_length;
				AVCodecContext *audio_ctx = context->streams[audio_index]->codec;

				// Get samples kept by resampler
				while(resampled_length = swr_convert(swr_ctx,
					resampled_data, resampled_alloc, 0, 0))
				{
					if(resampled_length < 0)
					{
						errormsg(_("FileAVlibs::close_file: failed to resample last data"));
						break;
					}
					write_samples(resampled_length, audio_ctx);
				}
				// Get out samples kept in encoder
				av_init_packet(&pkt);
				pkt.data = 0;
				pkt.size = 0;
				for(got_output = 1; got_output;)
				{
					if(avcodec_encode_audio2(audio_ctx, &pkt, 0, &got_output))
					{
						errormsg(_("FileAVlibs::close_file: failed to encode last audio packet"));
						break;
					}
					if(got_output)
					{
						pkt.stream_index = audio_index;
						if((rv = av_interleaved_write_frame(context, &pkt)) < 0)
						{
							liberror(rv, _("FileAVlibs::close_file: Failed to write last audio packet"));
							break;
						}
					}
				}
			}
			if(avvframe && headers_written)
			{
				AVPacket pkt;
				int got_output, rv;
				AVCodecContext *video_ctx = context->streams[video_index]->codec;

				av_init_packet(&pkt);
				pkt.data = 0;
				pkt.size = 0;

				for(got_output = 1; got_output;)
				{
					if(avcodec_encode_video2(video_ctx, &pkt, 0, &got_output))
					{
						errormsg(_("FileAVlibs::close_file: failed to encode last audio packet"));
						break;
					}
					if(got_output)
					{
						pkt.stream_index = video_index;
						if((rv = av_interleaved_write_frame(context, &pkt)) < 0)
						{
							liberror(rv, _("FileAVlibs::close_file: Failed to write last video packet"));
							break;
						}
					}
				}
			}
			if(headers_written)
				av_write_trailer(context);
			if(video_index >= 0)
				avcodec_close(context->streams[video_index]->codec);
			if(audio_index >= 0)
				avcodec_close(context->streams[audio_index]->codec);
			if(!(context->oformat->flags & AVFMT_NOFILE))
				avio_closep(&context->pb);
			avformat_free_context(context);
			context = 0;
		}
	}
	if(avvframe)
		av_frame_free(&avvframe);
	if(avaframe)
		av_frame_free(&avaframe);
	if(swr_ctx)
		swr_free(&swr_ctx);
	delete temp_frame;
	temp_frame = 0;
	avlibs_lock->unlock();
}

AVDictionary *FileAVlibs::create_dictionary(int options)
{
	AVDictionary *dict = 0;

	list2dictionary(&dict, asset->encoder_parameters[FILEAVLIBS_GLOBAL_IX]);
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

int FileAVlibs::read_frame(VFrame *frame)
{
	int res = 0;
	int error = 0;
	int got_it;
	int64_t rqpos;
	stream_item *itm;
	AVPacket pkt = {0};

	avlibs_lock->lock("AVlibs::read_frame");
	AVStream *stream = context->streams[video_index];
	AVCodecContext *decoder_context = stream->codec;

	if(!avvframe)
		avvframe = av_frame_alloc();
	rqpos = round((frame->get_source_pts() + pts_base) / av_q2d(stream->time_base));

	if(rqpos != video_pos)
	{
		itm = tocfile->get_item(video_index, rqpos);

		if(rqpos < video_pos || itm->index > video_pos)
		{
			if((res = avformat_seek_file(context, video_index,
				INT64_MIN, itm->offset, INT64_MAX,
				AVSEEK_FLAG_ANY)) < 0)
			{
				liberror(res, "FileAVlibs::read_frame:avformat_seek_file");
				avlibs_lock->unlock();
				return 1;
			}
			avcodec_flush_buffers(decoder_context);
			video_pos = itm->index;
		}
		while(video_pos < rqpos && av_read_frame(context, &pkt) == 0)
		{
			if(pkt.stream_index == video_index)
			{
				if((res = avcodec_decode_video2(decoder_context,
					avvframe, &got_it, &pkt)) < 0)
				{
					liberror(res, "FileAVlibs::read_frame:avcodec_decode_video2 skip");
					avlibs_lock->unlock();
					return 1;
				}
				av_free_packet(&pkt);

				if(got_it)
				{
					int64_t duration = av_frame_get_pkt_duration(avvframe);
					video_pos = avvframe->pkt_pts + duration;

					// rqpos is in the middle of next frame
					if(video_pos + duration > rqpos)
						break;
				}
			}
		}
	}

	while(1)
	{
		error = av_read_frame(context, &pkt);
		if(error)
		{
			if(error != AVERROR_EOF)
				break;
			error = 0;
		}
		if(pkt.stream_index == video_index)
		{
			if((res = avcodec_decode_video2(decoder_context,
				avvframe, &got_it, &pkt)) < 0)
			{
				liberror(res, "FileAVlibs::read_frame:avcodec_decode_video2 read");
				avlibs_lock->unlock();
				return 1;
			}
			av_free_packet(&pkt);

			if(got_it)
			{
				video_pos = avvframe->pkt_pts + av_frame_get_pkt_duration(avvframe);
				break;
			}
		}
		else
			av_free_packet(&pkt);
	}

	// Convert colormodel: Use ffmpeg, as it's not clear that the
	// quicktime code knows anything about alternate chroma siting
	if(!error && got_it)
	{
		AVPicture picture_out;
		PixelFormat pix_fmt = color_model_to_pix_fmt(frame->get_color_model());

		convert_cmodel((AVPicture *)avvframe, decoder_context->pix_fmt,
			decoder_context->width, decoder_context->height, frame);
		frame->set_source_pts(av_frame_get_best_effort_timestamp(avvframe) *
			av_q2d(stream->time_base) - pts_base);
		frame->set_duration(av_frame_get_pkt_duration(avvframe) * av_q2d(stream->time_base));
		frame->set_frame_number(round(frame->get_source_pts() * av_q2d(stream->avg_frame_rate)));
	}
	avlibs_lock->unlock();

	if(error)
	{
		liberror(error, "FileAVlibs::read_frame:av_read_frame");
		return 1;
	}

	return error;
}

int FileAVlibs::read_aframe(AFrame *aframe)
{
	int64_t rqpos, rqlen;
	AVStream *stream = context->streams[audio_index];
	AVCodecContext *decoder_context = stream->codec;
	int num_ch = decoder_context->channels;
	int error;

	avlibs_lock->lock("FileAVlibs::read_aframe");

	if(!swr_ctx)
	{
		swr_ctx = swr_alloc_set_opts(NULL,
			AV_CH_LAYOUT_STEREO,          // output layout
			AV_SAMPLE_FMT_DBLP,           // out sample format
			aframe->samplerate,           // out samplerate
			AV_CH_LAYOUT_STEREO,          // in ch layout
			decoder_context->sample_fmt,  // in sample fmt
			decoder_context->sample_rate, // in sample rate
			0, 0);
		if(!swr_ctx)
		{
			errorbox("FileAVlibs::fill_buffer: Failed to allocate resampler context");
			avlibs_lock->unlock();
			return -1;
		}
		if(error = swr_init(swr_ctx))
		{
			errorbox("FileAVlibs::fill_buffer: Failed to initalize resampler context (%d)", error);
			avlibs_lock->unlock();
			return -1;
		}
	}

	if(buffer_len < aframe->buffer_length || num_ch != num_buffers)
	{
		int j;
		for(j = 0; j < num_ch; j++)
		{
			if(abuffer[j])
				delete [] abuffer[j];
			abuffer[j] = new double[aframe->buffer_length];
		}
		for(; j < MAXCHANNELS; j++)
		{
			if(!abuffer[j])
				break;
			delete [] abuffer[j];
			abuffer[j] = 0;
		}
		num_buffers = num_ch;
		buffer_len = aframe->buffer_length;
		buffer_start = buffer_end = 0;
		buffer_pos = 0;
	}

	rqpos = round((aframe->source_pts + pts_base) / av_q2d(stream->time_base));
	rqlen = round(aframe->source_duration / av_q2d(stream->time_base));

	if(rqpos != buffer_start || (rqpos + rqlen > buffer_end && !audio_eof))
	{
		buffer_start = buffer_end = rqpos;
		buffer_pos = 0;

		if(error = decode_samples(rqpos, aframe->fill_length() - buffer_pos))
		{
			avlibs_lock->unlock();
			return error;
		}
		buffer_end = (double)buffer_pos / aframe->samplerate /
			av_q2d(stream->time_base) + buffer_start;
	}

	int copylen = MIN(buffer_pos, aframe->fill_length());
	aframe->set_filled(copylen);
	memcpy(aframe->buffer, abuffer[aframe->channel],
		copylen * sizeof(double));
	avlibs_lock->unlock();
	return 0;
}

int FileAVlibs::decode_samples(int64_t rqpos, int length)
{
	int error = 0;
	int got_it;
	int res = 0;
	AVPacket pkt = {0};
	stream_item *itm;
	AVCodecContext *decoder_context = context->streams[audio_index]->codec;

	if(!avaframe)
		avaframe = av_frame_alloc();

	if(rqpos != audio_pos - audio_delay)
	{
		audio_eof = 0;
		itm = tocfile->get_item(audio_index, rqpos);
		if(rqpos < audio_pos - audio_delay || (rqpos > audio_pos + 10
			&& audio_pos < itm->index))
		{
			if((res = avformat_seek_file(context, audio_index,
				INT64_MIN, itm->offset, INT64_MAX,
				AVSEEK_FLAG_ANY)) < 0)
			{
				liberror(res, "FileAVlibs::decode_samples:avformat_seek_file");
				return res;
			}
			swr_init(swr_ctx);
			avcodec_flush_buffers(decoder_context);
			audio_delay = 0;
			audio_pos = itm->index;
		}
		while(audio_pos < rqpos && av_read_frame(context, &pkt) == 0)
		{
			if(pkt.stream_index == audio_index)
			{
				if((res = avcodec_decode_audio4(decoder_context,
					avaframe, &got_it, &pkt)) < 0)
				{
					liberror(res, "FileAVlibs::decode_samples:avcodec_decode_audio skip");
					return res;
				}
				av_free_packet(&pkt);

				if(got_it)
				{
					int64_t duration = av_frame_get_pkt_duration(avaframe);

					audio_pos = av_frame_get_best_effort_timestamp(avaframe) +
						duration;

					if(audio_pos > rqpos)
					{
						int inpos = rqpos - audio_pos + duration;
						res = fill_buffer(avaframe,
							inpos,
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

	while(!audio_eof)
	{
		if(error = av_read_frame(context, &pkt))
		{
			if(error != AVERROR_EOF)
				break;
			error = 0;
			audio_eof = 1;
			pkt.stream_index = audio_index;
		}

		if(pkt.stream_index == audio_index)
		{
			if((res = avcodec_decode_audio4(decoder_context,
				avaframe, &got_it, &pkt)) < 0)
			{
				liberror(res, "FileAVlibs::decode_samples:avcodec_decode_audio read");
				return res;
			}
			av_free_packet(&pkt);

			if(got_it)
			{
				audio_pos = av_frame_get_best_effort_timestamp(avaframe) +
					av_frame_get_pkt_duration(avaframe);

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
		}
	}

	if(error)
	{
		liberror(error, "FileAVlibs::decode_samples");
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
	// FIXIT - 2 channels only
	if(!planar)
		inpos *= 2;

	for(int j = 0; j < AV_NUM_DATA_POINTERS; j++)
	{
		if(!avaframe->data[j])
			ibp[j] = 0;
		else
			ibp[j] = &avaframe->data[j][inpos];
	}
	in_length = av_frame_get_pkt_duration(avaframe) - insamples;
	out_samples = swr_convert(swr_ctx,
		(uint8_t **)obp,                   // output buffer
		buffer_len - buffer_pos,           // output buffer in samples
		(const uint8_t **)&ibp,            // input samples
		in_length); // input samples

	if(out_samples > 0)
		buffer_pos += out_samples;

	return out_samples;
}

int FileAVlibs::convert_cmodel(AVPicture *picture_in, PixelFormat pix_fmt_in,
	int width_in, int height_in, VFrame *frame_out)
{
	// set up a temporary picture_out from frame_out
	AVPicture picture_out;
	int cmodel_out = frame_out->get_color_model();
	PixelFormat pix_fmt_out = color_model_to_pix_fmt(cmodel_out);
	int result = 0;

	if(init_picture_from_frame(&picture_out, frame_out) >= 0)
	{
		if(pix_fmt_out != AV_PIX_FMT_NB)
		{
			sws_ctx = sws_getCachedContext(sws_ctx, width_in, height_in, pix_fmt_in,
				frame_out->get_w(),frame_out->get_h(),pix_fmt_out,
				SWS_BICUBIC, NULL, NULL, NULL);

			if(sws_ctx == NULL)
			{
				errorbox("FileAVlibs::convert_cmodel: swscale context initialization failed");
				return 1;
			}

			frame_out->clear_frame();
			sws_scale(sws_ctx,
				picture_in->data, picture_in->linesize,
				0, height_in,
				picture_out.data, picture_out.linesize);

			return result;
		}
	}

// we get here if there's no direct path from the FFMPEG
// pix_fmt_in to Cineleraa's/Quicktimes frame_out colormodel.
// So-- an intermediate conversion is called for

	if(cmodel_out == BC_RGBA8888)
	{
		// avoid infinite recursion if things are broken
		errorbox("FileAVlibs::convert_cmodel pix_fmt_in broken!");
		return 1;
	}

	if(temp_frame && (temp_frame->get_w() != frame_out->get_w() ||
		temp_frame->get_h() != frame_out->get_h()))
	{
		delete temp_frame;
		temp_frame = 0;
	}

	// choose RGBA8888 as a hopefully non-lossy colormodel
	if(!temp_frame)
		temp_frame = new VFrame(0, frame_out->get_w(), frame_out->get_h(),
			BC_RGBA8888);

	if(convert_cmodel(picture_in, pix_fmt_in,
			width_in, height_in, temp_frame))
		return 1;  // recursed call will print error message

	// if we reach here we know that cmodel_transfer() will work
	cmodel_transfer(// Packed data out
		frame_out->get_rows(),
		// Packed data in
		temp_frame->get_rows(),
		// Planar data out
		frame_out->get_y(), frame_out->get_u(), frame_out->get_v(),
		// Planar data in
		NULL,NULL,NULL,
		// Dimensions in
		0, 0, temp_frame->get_w(), temp_frame->get_h(),
		// Dimensions out
		0, 0, temp_frame->get_w(), temp_frame->get_h(),
		// Color model in, color model out
		BC_RGBA8888, cmodel_out,
		// Background color
		0,
		// Rowspans in, out (of luma for YUV)
		temp_frame->get_w(), temp_frame->get_w());
	return 0;
}

AVPixelFormat FileAVlibs::color_model_to_pix_fmt(int color_model)
{
	switch(color_model)
	{
	case BC_YUV422:
		return AV_PIX_FMT_YUYV422;
	case BC_RGB888:
		return AV_PIX_FMT_RGB24;
	case BC_BGR8888:
		return AV_PIX_FMT_BGRA;
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
		if(get_byte_order())
			return AV_PIX_FMT_RGB565LE;
		else
			return AV_PIX_FMT_RGB565LE;
	case BC_RGBA8888:
		return AV_PIX_FMT_RGBA;
	case BC_RGB8:
		return AV_PIX_FMT_RGB8;
	case BC_BGR565:
		if(get_byte_order())
			return AV_PIX_FMT_BGR565LE;
		else
			return AV_PIX_FMT_BGR565BE;
	case BC_ARGB8888:
		return AV_PIX_FMT_ARGB;
	case BC_ABGR8888:
		return AV_PIX_FMT_ABGR;
	case BC_RGB161616:
		if(get_byte_order())
			return AV_PIX_FMT_RGB48LE;
		else
			return AV_PIX_FMT_RGB48BE;
	};
	return AV_PIX_FMT_NB;
}

int FileAVlibs::init_picture_from_frame(AVPicture *picture, VFrame *frame)
{
	int cmodel = frame->get_color_model();
	PixelFormat pix_fmt = color_model_to_pix_fmt(cmodel);

	int size = avpicture_fill(picture, frame->get_data(), pix_fmt,
		frame->get_w(), frame->get_h());

	if(size < 0) return -1;

	if(cmodel_is_planar(frame->get_color_model()))
	{
// BC_YUV420P, BC_YUV422P, BC_YUV444P, BC_YUV411P:
		// override avpicture_fill() for planar types
		picture->data[0] = frame->get_y();
		picture->data[1] = frame->get_u();
		picture->data[2] = frame->get_v();
	}
	return size;
}

int FileAVlibs::streamformat(AVFormatContext *context)
{
	for(int i = 0; known_formats[i].decoder; i++)
	{
		if(strncmp(context->iformat->name, known_formats[i].decoder,
				strlen(known_formats[i].decoder)) == 0)
			return known_formats[i].fileformat;
	}
	return FILE_UNKNOWN;
}

const char *FileAVlibs::encoder_formatname(int fileformat)
{
	for(int i = 0; known_formats[i].encoder; i++)
	{
		if(known_formats[i].fileformat == fileformat)
			return known_formats[i].encoder;
	}
	return 0;
}

void FileAVlibs::liberror(int code, const char *prefix)
{
	int len;
	char string[BCTEXTLEN];

	strcpy(string, prefix);
	strcat(string, ": ");
	len = strlen(string);
	av_strerror(code, &string[len], BCTEXTLEN - 1 - len);
	errormsg("%s", string);
}

int FileAVlibs::write_frames(VFrame ***frames, int len)
{
	AVCodecContext *video_ctx;
	AVStream *stream;
	int got_it, rv;

	if(video_index < 0)
		return 1;

	avlibs_lock->lock("FileAVlibs::write_frames");

	stream = context->streams[video_index];
	video_ctx = stream->codec;

	for(int j = 0; j < len; j++)
	{
		VFrame *frame = frames[0][j];

		if(pts_base < 0)
			pts_base = frame->get_pts();

		if(!temp_frame)
		{
			temp_frame = new VFrame (0, video_ctx->width,
				video_ctx->height, BC_YUV420P);
		}
		cmodel_transfer(temp_frame->get_rows(),
			frame->get_rows(),
			temp_frame->get_y(),
			temp_frame->get_u(),
			temp_frame->get_v(),
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			0,
			0,
			frame->get_w(),
			frame->get_h(),
			0,
			0,
			temp_frame->get_w(),
			temp_frame->get_h(),
			frame->get_color_model(),
			BC_YUV420P,
			0,
			frame->get_w(),
			temp_frame->get_w());
		avvframe->data[0] = temp_frame->get_y();
		avvframe->data[1] = temp_frame->get_u();
		avvframe->data[2] = temp_frame->get_v();
		avvframe->linesize[0] = temp_frame->get_w();
		avvframe->linesize[1] = temp_frame->get_w() / 2;
		avvframe->linesize[2] = temp_frame->get_w() / 2;
		avvframe->pts = round((frame->get_pts() - pts_base) / av_q2d(stream->time_base));

		AVPacket pkt = {0};
		av_init_packet(&pkt);
		if((rv = avcodec_encode_video2(video_ctx, &pkt, avvframe, &got_it)) < 0)
		{
			liberror(rv, "FileAVlibs::write_frames: Failed to encode video frame");
			avlibs_lock->unlock();
			return 1;
		}
		if(got_it)
		{
			pkt.stream_index = stream->index;
			if((rv = av_interleaved_write_frame(context, &pkt)) < 0)
			{
				liberror(rv, "FileAVlibs::write_frames: Failed to write video_frame");
				avlibs_lock->unlock();
				return 1;
			}
		}
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
	int in_length = frames[0]->length;
	double *in_data[MAXCHANNELS];
	int resampled_length;

	if(audio_index < 0)
		return 1;

	avlibs_lock->lock("FileAVlibs::write_aframes");

	if(pts_base < 0)
		pts_base = frames[0]->pts;

	stream = context->streams[audio_index];
	audio_ctx = stream->codec;

	if(resampled_alloc < in_length)
	{
		int sample_bytes = av_get_bytes_per_sample(audio_ctx->sample_fmt);

		resampled_alloc = av_rescale_rnd(swr_get_delay(swr_ctx, asset->sample_rate) +
			in_length, audio_ctx->sample_rate, asset->sample_rate,
			AV_ROUND_UP);
		resampled_alloc = resampled_alloc / audio_ctx->frame_size * audio_ctx->frame_size;
		if(av_sample_fmt_is_planar(audio_ctx->sample_fmt))
		{
			for(chan = 0; chan < audio_ctx->channels; chan++)
				resampled_data[chan] = new uint8_t[resampled_alloc * sample_bytes];
		}
		else
			resampled_data[0] = new uint8_t[resampled_alloc * sample_bytes * audio_ctx->channels];
	}

// Resample the whole buffer
	for(chan = 0; chan < asset->channels; chan++)
		in_data[chan] = frames[chan]->buffer;

	if((resampled_length = swr_convert(swr_ctx, resampled_data,
		resampled_alloc, (const uint8_t**)in_data, in_length)) < 0)
	{
		errormsg("FileAVlibs::write_aframes:Failed to resample data");
		avlibs_lock->unlock();
		return 1;
	}
	rv = write_samples(resampled_length, audio_ctx, frames[0]->pts - pts_base);

// We put more samples into resampler than we consume,
// get some of them to avoid resampler become too large
	if(!rv && swr_get_out_samples(swr_ctx, 0) > resampled_alloc)
	{
		if((resampled_length = swr_convert(swr_ctx, resampled_data,
			resampled_alloc, 0, 0)) < 0)
		{
			errorbox(_("FileAVlibs::write_aframes:Failed to resample data"));
			avlibs_lock->unlock();
			return 1;
		}
		rv = write_samples(resampled_length, audio_ctx);
	}
	avlibs_lock->unlock();
	return rv;
}

int FileAVlibs::write_samples(int resampled_length, AVCodecContext *audio_ctx, ptstime pts)
{
	AVPacket pkt;
	int got_output, chan, rv;
	int frame_size = audio_ctx->frame_size;
	int linesize;
	int buffersize = av_samples_get_buffer_size(&linesize, audio_ctx->channels,
		frame_size, audio_ctx->sample_fmt, 1);

	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	if(!(pts < 0))
		audio_pos = pts / av_q2d(audio_ctx->time_base);

	for(int i = 0; i < resampled_length; i += frame_size)
	{
		int sample_bytes = av_get_bytes_per_sample(audio_ctx->sample_fmt);

		avaframe->nb_samples = frame_size;
		avaframe->format = audio_ctx->sample_fmt;
		avaframe->channel_layout = audio_ctx->channel_layout;
		avaframe->pts = audio_pos;

		if(av_sample_fmt_is_planar(audio_ctx->sample_fmt))
		{
			for(chan = 0; chan < audio_ctx->channels; chan++)
				avaframe->data[chan] = &resampled_data[chan][i * sample_bytes];
		}
		else
			avaframe->data[0] = &resampled_data[0][i * sample_bytes * audio_ctx->channels];

		avaframe->linesize[0] = linesize;

		if((rv = avcodec_encode_audio2(audio_ctx, &pkt, avaframe, &got_output)) < 0)
		{
			liberror(rv, _("FileAVlibs::write_samples:Failed encoding audio frame"));
			return 1;
		}
		audio_pos += frame_size;
		if(got_output)
		{
			pkt.stream_index = audio_index;

			if((rv = av_interleaved_write_frame(context, &pkt)) < 0)
			{
				liberror(rv, _("FileAVlibs::write_aframes: Failed to write audio frame"));
				return 1;
			}
		}
	}
	av_free_packet(&pkt);
	return 0;
}

// Callbacks for TOC generation
int FileAVlibs::get_streamcount()
{
	return context->nb_streams;
}

stream_params *FileAVlibs::get_track_data(int trx)
{
	AVStream *stream;
	AVCodecContext *decoder_context;
	AVPacket pkt;
	int got_it;
	int64_t pktpos;
	int interrupt = 0;
	int is_audio;
	int res, err;

	if(trx >= context->nb_streams)
		return 0;

	stream = context->streams[trx];
	decoder_context = stream->codec;

	switch(decoder_context->codec_type)
	{
	case AVMEDIA_TYPE_AUDIO:
		track_data.type = TOCSTREAM_AUDIO;
		track_data.rate_num = decoder_context->sample_rate;
		track_data.rate_denom = 0;
		track_data.layers = decoder_context->channels;
		break;

	case AVMEDIA_TYPE_VIDEO:
		track_data.type = TOCSTREAM_VIDEO;
		track_data.rate_num = stream->r_frame_rate.num;
		track_data.rate_denom = stream->r_frame_rate.den;
		track_data.layers = 1;
		break;
	}
	track_data.id = trx;
	track_data.data1 = 0;
	track_data.data2 = 0;
	track_data.min_index = INT64_MAX;
	track_data.min_offset = 0;
	track_data.index_correction = 0;
	err = avformat_seek_file(context, trx, INT64_MIN, INT64_MIN, 0,
		AVSEEK_FLAG_ANY);

	avcodec_flush_buffers(decoder_context);
	avformat_flush(context);

	pkt.buf = 0;
	pkt.size = 0;

	while((err = av_read_frame(context, &pkt)) == 0)
	{
		if(pkt.stream_index == trx)
		{
			switch(decoder_context->codec_type)
			{
			case AVMEDIA_TYPE_VIDEO:
				video_pos = pkt.pts;
				pktpos = pkt.dts;

				if(pkt.flags & AV_PKT_FLAG_KEY)
				{
					interrupt = tocfile->append_item(video_pos, pktpos);
					if(video_pos < track_data.min_index)
					{
						track_data.min_index = video_pos;
						track_data.min_offset = pktpos;
					}
				}
				video_pos += pkt.duration;
				pktpos += pkt.duration;
				av_free_packet(&pkt);
				is_audio = 0;
				break;

			case AVMEDIA_TYPE_AUDIO:
				audio_pos = pkt.pts;
				pktpos = pkt.dts;

				if(pkt.flags & AV_PKT_FLAG_KEY)
				{
					interrupt = tocfile->append_item(audio_pos, pktpos);
					if(audio_pos < track_data.min_index)
					{
						track_data.min_index = audio_pos;
						track_data.min_offset = pktpos;
					}
				}
				audio_pos += pkt.duration;
				pktpos += pkt.duration;
				av_free_packet(&pkt);
				is_audio = 1;
				break;
			}
			if(interrupt)
				return 0;
		}
	}

	if(err != AVERROR_EOF)
		return 0;

	track_data.max_offset = pktpos;
	track_data.max_index = (is_audio ? audio_pos : video_pos) + track_data.index_correction;

	return &track_data;
}

void FileAVlibs::versionifo(int indent)
{
#define PRINT_LIB_INFO(libname, LIBNAME) \
	{ \
		unsigned int version = libname##_version(); \
		fprintf(stderr, "%*slib%-11s %2d.%3d.%3d / %2d.%3d.%3d\n", \
			indent, "", #libname, \
			LIB##LIBNAME##_VERSION_MAJOR, \
			LIB##LIBNAME##_VERSION_MINOR, \
			LIB##LIBNAME##_VERSION_MICRO, \
			version >> 16, \
			version >> 8 & 0xff, \
			version & 0xff); \
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
	Paramlist *defaults;

	FileAVlibs::avlibs_lock->lock("AVlibsConfig::AVlibsConfig");
	avcodec_register_all();
	av_register_all();
	FileAVlibs::avlibs_lock->unlock();

	AVlibsConfig *window = new AVlibsConfig(asset, options);
	format_window = window;
	window->run_window();
	defaults = scan_global_options(options);
	window->save_options(window->globopts, FILEAVLIBS_GLOBAL_CONFIG, 0, defaults);
	if(asset->encoder_parameters[FILEAVLIBS_GLOBAL_IX])
	{
		delete asset->encoder_parameters[FILEAVLIBS_GLOBAL_IX];
		asset->encoder_parameters[FILEAVLIBS_GLOBAL_IX] = 0;
	}
	if(window->globopts->total() > 0)
	{
		window->globopts->clean_list();
		asset->encoder_parameters[FILEAVLIBS_GLOBAL_IX] = window->globopts;
		window->globopts = 0;
	}
	delete defaults;

	defaults = scan_format_options(asset->format, options, 0);
	window->save_options(window->fmtopts, FILEAVLIBS_FORMAT_CONFIG,
		window->fmtopts->name, defaults);
	if(asset->encoder_parameters[FILEAVLIBS_FORMAT_IX])
	{
		delete asset->encoder_parameters[FILEAVLIBS_FORMAT_IX];
		asset->encoder_parameters[FILEAVLIBS_FORMAT_IX] = 0;
	}
	if(window->fmtopts->total() > 0)
	{
		window->fmtopts->clean_list();
		asset->encoder_parameters[FILEAVLIBS_FORMAT_IX] = window->fmtopts;
		window->fmtopts = 0;
	}
	delete defaults;

	if(window->codecopts)
	{
		Paramlist **alist, **plist;
		Param *pa;

		const char *pfx = options & SUPPORTS_VIDEO ?
				FILEAVLIBS_VCODEC_CONFIG : FILEAVLIBS_ACODEC_CONFIG;
		defaults = scan_encoder_opts((AVCodecID)window->current_codec, options);
		window->save_options(window->codecopts,
			pfx, window->codecopts->name, defaults);
		if(options & SUPPORTS_VIDEO)
		{
			pa = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->set(PARAM_CODEC_VIDEO, window->current_codec);
			pa->set(window->codecopts->name);
			strcpy(asset->vcodec, window->codecopts->name);
			alist = &asset->encoder_parameters[FILEAVLIBS_VCODEC_IX];
			plist = &asset->encoder_parameters[FILEAVLIBS_VPRIVT_IX];
		}
		else if(options & SUPPORTS_AUDIO)
		{
			pa = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->set(PARAM_CODEC_AUDIO, window->current_codec);
			pa->set(window->codecopts->name);
			strcpy(asset->acodec, window->codecopts->name);
			alist = &asset->encoder_parameters[FILEAVLIBS_ACODEC_IX];
			plist = &asset->encoder_parameters[FILEAVLIBS_APRIVT_IX];
		}
		if(*alist)
		{
			delete *alist;
			*alist = 0;
		}
		if(*plist)
		{
			delete *plist;
			*plist = 0;
		}
		if(window->codecopts->total() > 0)
		{
			window->codecopts->clean_list();
			*alist = window->codecopts;
			window->codecopts = 0;
		}
		delete defaults;
		if(*plist)
		{
			delete *plist;
			*plist = 0;
		}
		if(window->codec_private)
		{
			pfx = options & SUPPORTS_VIDEO ?
				FILEAVLIBS_VPRIVT_CONFIG : FILEAVLIBS_APRIVT_CONFIG;
			defaults = scan_encoder_private_opts((AVCodecID)window->current_codec, options);
			window->save_options(window->codec_private,
				pfx, window->codec_private->name, defaults);
			window->codec_private->clean_list(),
			*plist = window->codec_private;
			window->codec_private = 0;
			delete defaults;
		}
		window->save_options(asset->encoder_parameters[FILEAVLIBS_CODECS_IX],
			FILEAVLIBS_CODECS_CONFIG, window->fmtopts->name);
	}

	delete window;
}

void FileAVlibs::get_render_defaults(Asset *asset)
{
	const char *name;
	AVOutputFormat *oformat = 0;
	AVCodec *encoder;
	Param *apar;

	if(!(name = FileAVlibs::encoder_formatname(asset->format)))
		return;

	asset->encoder_parameters[FILEAVLIBS_GLOBAL_IX] = AVlibsConfig::load_options(FILEAVLIBS_GLOBAL_CONFIG);
	asset->encoder_parameters[FILEAVLIBS_FORMAT_IX] = AVlibsConfig::load_options(FILEAVLIBS_FORMAT_CONFIG, name);
	asset->encoder_parameters[FILEAVLIBS_CODECS_IX] = AVlibsConfig::load_options(FILEAVLIBS_CODECS_CONFIG, name);

	FileAVlibs::avlibs_lock->lock("AVlibsConfig::AVlibsConfig");
	avcodec_register_all();
	av_register_all();
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
		asset->acodec[0] = 0;

		if(oformat->audio_codec == AV_CODEC_ID_NONE)
			codecs->remove_param(PARAM_CODEC_AUDIO);
		else if(!(apar = codecs->find(PARAM_CODEC_AUDIO)) ||
				!(encoder = avcodec_find_encoder((AVCodecID)apar->intvalue)))
		{
			if(encoder = avcodec_find_encoder(oformat->audio_codec))
			{
				apar = codecs->set(PARAM_CODEC_AUDIO, oformat->audio_codec);
				apar->set(encoder->name);
			}
		}
		if(apar)
			strncpy(asset->acodec, encoder->name, MAX_LEN_CODECNAME);

		apar = 0;
		asset->vcodec[0] = 0;
		if(oformat->video_codec == AV_CODEC_ID_NONE)
			codecs->remove_param(PARAM_CODEC_VIDEO);
		else if(!(apar = codecs->find(PARAM_CODEC_VIDEO)) ||
				!(encoder = avcodec_find_encoder((AVCodecID)apar->intvalue)))
		{
			if(encoder = avcodec_find_encoder(oformat->video_codec))
			{
				apar = codecs->set(PARAM_CODEC_VIDEO, oformat->video_codec);
				apar->set(encoder->name);
			}
		}
		if(apar)
			strncpy(asset->vcodec, encoder->name, MAX_LEN_CODECNAME);
		asset->vcodec[MAX_LEN_CODECNAME - 1] = 0;
		asset->acodec[MAX_LEN_CODECNAME - 1] = 0;
	}
	asset->encoder_parameters[FILEAVLIBS_ACODEC_IX] = AVlibsConfig::load_options(FILEAVLIBS_ACODEC_CONFIG, asset->acodec);
	asset->encoder_parameters[FILEAVLIBS_VCODEC_IX] = AVlibsConfig::load_options(FILEAVLIBS_VCODEC_CONFIG, asset->vcodec);
	asset->encoder_parameters[FILEAVLIBS_APRIVT_IX] = AVlibsConfig::load_options(FILEAVLIBS_APRIVT_CONFIG, asset->acodec);
	asset->encoder_parameters[FILEAVLIBS_VPRIVT_IX] = AVlibsConfig::load_options(FILEAVLIBS_VPRIVT_CONFIG, asset->vcodec);
}

Paramlist *FileAVlibs::scan_global_options(int options)
{
	return scan_options(avformat_get_class(), 0, "AVlibsGlobal");
}

Paramlist *FileAVlibs::scan_format_options(int format,
	int options, AVOutputFormat **ofmtp)
{
	const char *name;
	AVOutputFormat *oformat = 0;

	if(ofmtp)
		*ofmtp = 0;
	name = encoder_formatname(format);

	if(name)
	{
		oformat = av_guess_format(name, 0, 0);
		if(ofmtp)
			*ofmtp = oformat;
		if(oformat)
			return scan_options(oformat->priv_class, 0, name);
	}
	return 0;
}

Paramlist *FileAVlibs::scan_options(const AVClass *avclass, int options, const char *listname)
{
	int typefl = AV_OPT_FLAG_ENCODING_PARAM;
	const AVOption *opt = 0;
	Param *param;
	Paramlist *list;

	list = new Paramlist(listname);

	if(options & SUPPORTS_AUDIO)
		typefl |= AV_OPT_FLAG_AUDIO_PARAM;
	if(options & SUPPORTS_VIDEO)
		typefl |= AV_OPT_FLAG_VIDEO_PARAM;

	while(opt = av_opt_next(&avclass, opt))
	{
		if(opt->type == AV_OPT_TYPE_CONST || skip_avoption(opt, typefl))
			continue;
		param = opt2param(list, opt);
		if(opt->unit)
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
	return 0;
}

Paramlist *FileAVlibs::scan_codecs(AVOutputFormat *oformat, int options)
{
	const struct AVCodecTag * const *ctag;
	const struct AVCodecTag *tags;
	AVCodec *encoder;
	Param *param;
	Paramlist *codecs;

	codecs = new Paramlist("AVLibCodecs");
	if(options & SUPPORTS_VIDEO)
		codecs->selectedint = oformat->video_codec;
	if(options & SUPPORTS_AUDIO)
		codecs->selectedint = oformat->audio_codec;

	for(ctag = oformat->codec_tag; *ctag; ctag++)
	{
		for(tags = *ctag; tags->id != AV_CODEC_ID_NONE; tags++)
		{
			encoder = avcodec_find_encoder(tags->id);
			if(encoder)
			{
				if(options & SUPPORTS_AUDIO && encoder->type != AVMEDIA_TYPE_AUDIO)
					continue;
				if(options & SUPPORTS_VIDEO && encoder->type != AVMEDIA_TYPE_VIDEO)
					continue;
				if(codecs->find(encoder->name))
					continue;
				param = codecs->append_param(encoder->name, tags->id);
				param->set_help(encoder->long_name);
			}
		}
	}
	return codecs;
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
	return libopts;
}

Paramlist *FileAVlibs::scan_encoder_private_opts(AVCodecID codec, int options)
{
	AVCodec *encoder;

	encoder = avcodec_find_encoder(codec);

	if(encoder && encoder->priv_class)
		return scan_options(encoder->priv_class, options, encoder->name);

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
		for(int i = 0; codec->sample_fmts[i]; i++)
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
	printf("%*sAVOutputformat %p dump:\n", indent, "", ofmt);
	indent += 2;
	printf("%*sname: '%s' mime_type: '%s'\n", indent, "", ofmt->name, ofmt->mime_type);
	printf("%*slong_name '%s'\n", indent, "", ofmt->long_name);
	printf("%*saudio_codec '%s' video_codec '%s' subtitle_codec '%s' data_codec '%s'\n", indent, "",
		avcodec_get_name(ofmt->audio_codec), avcodec_get_name(ofmt->video_codec),
		avcodec_get_name(ofmt->subtitle_codec), avcodec_get_name(ofmt->data_codec));
	printf("%*sflags %#x codec_tag %p priv_class %p\n", indent, "",
		ofmt->flags, ofmt->codec_tag, ofmt->priv_class);
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
	case  AV_OPT_TYPE_CHANNEL_LAYOUT:
		return "chnl_layout";
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
	printf("%*sfilename '%s'\n", indent, "", ctx->filename);
	printf("%*sstart_time %s(%.2f) duration %s(%.2f), bit_rate %d\n", indent, "",
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
	printf("%*sindex %d id %d codec %p priv_data %p\n", indent, "",
		stm->index, stm->id, stm->codec, stm->priv_data);
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
	printf("%*sevent_flags %d info %p first_dts %s cur_dts %s\n", indent, "",
		stm->event_flags, stm->info, dump_ts(stm->first_dts, bf1),
		dump_ts(stm->cur_dts, bf2));
	printf("%*slast_IP_pts %s last_IP_duration %d probe_packets %d codec_info_nb_frames %d\n", indent, "",
		dump_ts(stm->last_IP_pts), stm->last_IP_duration, stm->probe_packets,
		stm->codec_info_nb_frames);
	printf("%*sneed_parsing %d last_in_packet_buffer %p\n", indent, "",
		stm->need_parsing, stm->last_in_packet_buffer);
	printf("%*sindex_entries %p nb_index_entries %d\n", indent, "",
		stm->index_entries, stm->nb_index_entries);
	printf("%*sindex_entries_allocated_size %u r_frame_rate %s stream_identifier %d\n", indent, "",
		stm->index_entries_allocated_size, dump_AVRational(&stm->r_frame_rate),
		stm->stream_identifier);
	printf("%*sinterleaver_chunk_size %s interleaver_chunk_duration %s request_probe %d\n", indent, "",
		dump_ts(stm->interleaver_chunk_size, bf1),
		dump_ts(stm->interleaver_chunk_duration, bf2), stm->request_probe);
	printf("%*sskip_to_keyframe %d skip_samples %d start_skip_samples %" PRId64 " first_discard_sample %" PRId64 "\n", indent, "",
		stm->skip_to_keyframe, stm->skip_samples, stm->start_skip_samples,
		stm->first_discard_sample);
	printf("%*slast_discard_sample %" PRId64 " nb_decoded_frames %d mux_ts_offset %" PRId64 "\n", indent, "",
		stm->last_discard_sample, stm->nb_decoded_frames, stm->mux_ts_offset);
	printf("%*spts_wrap_reference %s pts_wrap_behavior %d update_initial_durations_done %d\n", indent, "",
		dump_ts(stm->pts_wrap_reference), stm->pts_wrap_behavior,
		stm->update_initial_durations_done);
	printf("%*sinject_global_side_data %d recommended_encoder_configuration %p\n", indent, "",
		stm->inject_global_side_data, stm->recommended_encoder_configuration);
	printf("%*sdisplay_aspect_ratio %s\n", indent, "",
		dump_AVRational(&stm->display_aspect_ratio));
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
	printf("%*scodec_id %s fourcc %s priv_data %p, internal %p\n", indent, "",
		avcodec_get_name(ctx->codec_id), dump_fourcc(ctx->codec_tag),
		ctx->priv_data, ctx->internal);
	printf("%*sopaque %p bit_rate %d bit_rate_tolerance %d global_quality %d\n", indent, "",
		ctx->opaque, ctx->bit_rate, ctx->bit_rate_tolerance, ctx->global_quality);
	printf("%*scompression_level %d flags %d flags2 %d extradata %p extradata_size %d\n", indent, "",
		ctx->compression_level, ctx->flags, ctx->flags2, ctx->extradata,
		ctx->extradata_size);
	printf("%*stime_base %s ticks_per_frame %d delay %d w/h [%d,%d] coded [%d, %d]\n", indent, "",
		dump_AVRational(&ctx->time_base), ctx->ticks_per_frame, ctx->delay,
		ctx->width, ctx->height, ctx->coded_width, ctx->coded_height);
	printf("%*sgop_size %d, pix_fmt %d draw_horiz_band %p get_format %p\n", indent, "",
		ctx->gop_size, ctx->pix_fmt, ctx->draw_horiz_band, ctx->get_format);
	printf("%*smax_b_frames %d b_quant_factor %.2f b_frame_strategy %d b_quant_offset %.2f\n", indent, "",
		ctx->max_b_frames, ctx->b_quant_factor, ctx->b_frame_strategy, ctx->b_quant_offset);
	printf("%*shas_b_frames %d mpeg_quant %d i_quant_factor %.2f i_quant_offset %.2f\n", indent, "",
		ctx->has_b_frames, ctx->mpeg_quant, ctx->i_quant_factor, ctx->i_quant_offset);
	printf("%*slumi_masking %.2f temporal_cplx_masking %.2f spatial_cplx_masking %.2f\n", indent, "",
		ctx->lumi_masking, ctx->temporal_cplx_masking, ctx->spatial_cplx_masking);
	printf("%*sp_masking %.2f dark_masking %.2f slice_count %d prediction_method %d\n", indent, "",
		ctx->p_masking, ctx->dark_masking, ctx->slice_count, ctx->prediction_method);
	printf("%*sslice_offset %p sample_aspect_ratio %s me_cmp %d me_sub_cmp %d\n", indent, "",
		ctx->slice_offset, dump_AVRational(&ctx->sample_aspect_ratio),
		ctx->me_cmp, ctx->me_sub_cmp);
	printf("%*smb_cmp %d ildct_cmp %d dia_size %d last_predictor_count %d pre_me %d\n", indent, "",
		ctx->mb_cmp, ctx->ildct_cmp, ctx->dia_size, ctx->last_predictor_count,
		ctx->pre_me);
	printf("%*sme_pre_cmp %d pre_dia_size %d me_subpel_quality %d me_range %d\n",
		indent, "",
		ctx->me_pre_cmp, ctx->pre_dia_size, ctx->me_subpel_quality, ctx->me_range);
	printf("%*sslice_flags %d mb_decision %d intra_matrix %p inter_matrix %p\n", indent, "",
		ctx->slice_flags, ctx->mb_decision, ctx->intra_matrix, ctx->inter_matrix);
	printf("%*sscenechange_threshold %d, noise_reduction %d intra_dc_precision %d\n", indent, "",
		ctx->scenechange_threshold, ctx->noise_reduction, ctx->intra_dc_precision);
	printf("%*sskip_top %d skip_bottom %d mb_lmin %d mb_lmax %d me_penalty_compensation %d\n", indent, "",
		ctx->skip_top, ctx->skip_bottom, ctx->mb_lmin, ctx->mb_lmax,
		ctx->me_penalty_compensation);
	printf("%*sbidir_refine %d brd_scale %d keyint_min %d refs %d chromaoffset %d\n", indent, "",
		ctx->bidir_refine, ctx->brd_scale, ctx->keyint_min, ctx->refs,
		ctx->chromaoffset);
	printf("%*smv0_threshold %d b_sensitivity %d color_primaries %d color_trc %d\n", indent, "",
		ctx->mv0_threshold, ctx->b_sensitivity, ctx->color_primaries, ctx->color_trc);
	printf("%*scolorspace %d color_range %d chroma_sample_location %d slices %d\n", indent, "",
		ctx->colorspace, ctx->color_range, ctx->chroma_sample_location, ctx->slices);
	printf("%*sfield_order %d sample_rate %d channels %d, sample_fmt %d\n", indent, "",
		ctx->field_order, ctx->sample_rate, ctx->channels, ctx->sample_fmt);
	printf("%*sframe_size %d frame_number %d block_align %d cutoff %d\n", indent, "",
		ctx->frame_size, ctx->frame_number, ctx->block_align, ctx->cutoff);
	printf("%*schannel_layout %#" PRIx64 " request_channel_layout %#" PRIx64 " audio_service_type %d\n", indent, "",
		ctx->channel_layout, ctx->request_channel_layout, ctx->audio_service_type);
	printf("%*srequest_sample_fmt %d get_buffer2 %p refcounted_frames %d\n", indent, "",
		ctx->request_sample_fmt, ctx->get_buffer2, ctx->refcounted_frames);
	printf("%*sqcompress %.2f qblur %.2f qmin %d qmax %d max_qdiff %d\n", indent, "",
		ctx->qcompress, ctx->qblur, ctx->qmin, ctx->qmax, ctx->max_qdiff);
	printf("%*src_buffer_size %d rc_override_count %d rc_override %p\n", indent, "",
		ctx->rc_buffer_size, ctx->rc_override_count, ctx->rc_override);
	printf("%*src_max_rate %d rc_min_rate %d rc_max_available_vbv_use %.2f\n", indent, "",
		ctx->rc_max_rate, ctx->rc_min_rate, ctx->rc_max_available_vbv_use);
	printf("%*src_min_vbv_overflow_use %.2f rc_initial_buffer_occupancy %d\n", indent, "",
		ctx->rc_min_vbv_overflow_use, ctx->rc_initial_buffer_occupancy);
	printf("%*scoder_type %d context_model %d frame_skip_threshold %d\n", indent, "",
		ctx->coder_type, ctx->context_model, ctx->frame_skip_threshold);
	printf("%*sframe_skip_factor %d frame_skip_exp %d frame_skip_cmp %d\n", indent, "",
		ctx->frame_skip_factor, ctx->frame_skip_exp, ctx->frame_skip_cmp);
	printf("%*strellis %d min_prediction_order %d max_prediction_order %d\n", indent, "",
		ctx->trellis, ctx->min_prediction_order, ctx->max_prediction_order);
	printf("%*stimecode_frame_start %" PRId64 " rtp_callback %p rtp_payload_size %d\n", indent, "",
		ctx->timecode_frame_start, ctx->rtp_callback, ctx->rtp_payload_size);
	printf("%*smv_bits %d header_bits %d i_tex_bits %d p_tex_bits %d\n", indent, "",
		ctx->mv_bits, ctx->header_bits, ctx->i_tex_bits, ctx->p_tex_bits);
	printf("%*si_count %d p_count %d skip_count %d misc_bits %d\n", indent, "",
		ctx->i_count, ctx->p_count, ctx->skip_count, ctx->misc_bits);
	printf("%*sframe_bits %d stats_out %p stats_in %p, workaround_bugs %#x\n", indent, "",
		ctx->frame_bits, ctx->stats_out, ctx->stats_in, ctx->workaround_bugs);
	printf("%*sstrict_std_compliance %d error_concealment %d debug %#x\n", indent, "",
		ctx->strict_std_compliance, ctx->error_concealment, ctx->debug);
	printf("%*serr_recognition %#x reordered_opaque %s hwaccel %p hwaccel_context %p\n", indent, "",
		ctx->err_recognition, dump_ts(ctx->reordered_opaque), ctx->hwaccel,
		ctx->hwaccel_context);
	printf("%*sdct_algo %d idct_algo %d bits_per_coded_sample %d bits_per_raw_sample %d\n", indent, "",
		ctx->dct_algo, ctx->idct_algo, ctx->bits_per_coded_sample, ctx->bits_per_raw_sample);
	printf("%*sthread_count %d thread_type %d active_thread_type %d thread_safe_callbacks %d\n", indent, "",
		ctx->thread_count, ctx->thread_type, ctx->active_thread_type,
		ctx->thread_safe_callbacks);
	printf("%*sexecute %p execute2 %p nsse_weight %d profile %d level %d\n", indent, "",
		ctx->execute, ctx->execute2, ctx->nsse_weight, ctx->profile, ctx->level);
	printf("%*sskip_loop_filter %d skip_idct %d skip_frame %d\n", indent, "",
		ctx->skip_loop_filter, ctx->skip_idct, ctx->skip_frame);
	printf("%*ssubtitle_header %p subtitle_header_size %d vbv_delay %" PRId64 "\n", indent, "",
		ctx->subtitle_header, ctx->subtitle_header_size, ctx->vbv_delay);
	printf("%*sside_data_only_packets %d initial_padding %d framerate %s\n", indent, "",
		ctx->side_data_only_packets, ctx->initial_padding, dump_AVRational(&ctx->framerate));
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

	printf("%*sextended_data %p [%d,%d] nb_samples %d format %d\n", indent, "",
		avf->extended_data, avf->width, avf->height, avf->nb_samples, avf->format);
	printf("%*skey_frame %d pict_type %d sample_aspect_ratio %s pts %s\n", indent, "",
		avf->key_frame, avf->pict_type, dump_AVRational(&avf->sample_aspect_ratio),
		dump_ts(avf->pts));
	printf("%*spkt_pts %s pkt_dts %s coded_picture_number %d display_picture_number %d\n", indent, "",
		dump_ts(avf->pkt_pts, bf1), dump_ts(avf->pkt_dts, bf2),
		avf->coded_picture_number, avf->display_picture_number);
	printf("%*squality %d opaque %p repeat_pict %d interlaced_frame %d top_field_first %d\n", indent, "",
		avf->quality, avf->opaque, avf->repeat_pict, avf->interlaced_frame,
		avf->top_field_first);
	printf("%*spalette_has_changed %d reordered_opaque %s sample_rate %d\n", indent, "",
		avf->palette_has_changed, dump_ts(avf->reordered_opaque), avf->sample_rate);
	printf("%*schannel_layout %#" PRIx64 " extended_buf %p nb_extended_buf %d\n", indent, "",
		avf->channel_layout, avf->extended_buf, avf->nb_extended_buf);
	printf("%*sside_data %p nb_side_data %d flags %d color_range %d color_primaries %d\n", indent, "",
		avf->side_data, avf->nb_side_data, avf->flags, avf->color_range, avf->color_primaries);
	printf("%*scolor_trc %d colorspace %d chroma_location %d best_effort_timestamp %s\n", indent, "",
		avf->color_trc, avf->colorspace, avf->chroma_location, dump_ts(avf->best_effort_timestamp));
	printf("%*spkt_pos %s pkt_duration %s metadata %p decode_error_flags %d\n", indent, "",
		dump_ts(avf->pkt_pos, bf1), dump_ts(avf->pkt_duration, bf2),
		avf->metadata, avf->decode_error_flags);
	printf("%*schannels %d pkt_size %d qp_table_buf %p\n", indent, "",
		avf->channels, avf->pkt_size, avf->qp_table_buf);
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
	printf("%*sconvergence_duration %s\n", indent, "",
		dump_ts(pkt->convergence_duration));
}
