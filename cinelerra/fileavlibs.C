
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
#include "preferences.h"
#include "selection.h"
#include "videodevice.inc"
#include "vframe.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

struct selection_int FileAVlibs::known_formats[] =
{
	{ "mov,mp4,m4a,3gp,3g2,mj2", FILE_MOV },
	{ "avi", FILE_AVI },
	{ 0, 0 }
};

extern "C"
{
#include <libswscale/swscale.h>
}

Mutex* FileAVlibs::avlibs_lock = new Mutex("FileAVlibs::avlibs_lock");

FileAVlibs::FileAVlibs(Asset *asset, File *file)
 : FileBase(asset, file)
{
	avvframe = 0;
	avaframe = 0;
	audio_index = -1;
	video_index = -1;
	context = 0;
	sws_ctx = 0;
	swr_ctx = 0;
	temp_frame = 0;
	tocfile = 0;
	av_log_set_level(AV_LOG_QUIET);
	memset(&track_data, 0, sizeof(track_data));
	num_buffers = 0;
	buffer_len = 0;
	memset(abuffer, 0, sizeof(abuffer));
}

FileAVlibs::~FileAVlibs()
{
	if(avvframe) av_frame_free(&avvframe);
	if(avaframe) av_frame_free(&avaframe);
	delete temp_frame;
	for(int i = 0; i < MAXCHANNELS; i++)
		delete [] abuffer[i];
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

	avlibs_lock->lock("FileAVlibs::open_file");
	avcodec_register_all();
	av_register_all();

	audio_pos = 0;
	video_pos = 0;

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
							fprintf(stderr,"FileFFMPEG::open_file: audio codec 0x%x not found.\n",
									decoder_context->codec_id);
							asset->audio_data = 0;
							audio_index = -1;
						}
						else
						{
							avcodec_open2(decoder_context, codec, NULL);
						}
						strncpy(asset->acodec, codec->name, 4);
						asset->bits = av_get_bytes_per_sample(decoder_context->sample_fmt) * 8;
						audio_delay = 0;
						memset(buffer_valid, 0, sizeof(buffer_valid));
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
						avcodec_open2(decoder_context, codec, NULL);
						strncpy(asset->vcodec, codec->name, 4);
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
				asset->video_length = av_rescale_q(tocfile->toc_streams[video_index].max_index -
						tocfile->toc_streams[video_index].min_index,
						context->streams[video_index]->time_base,
						av_inv_q(context->streams[video_index]->r_frame_rate));
			}
			if(audio_index >= 0)
			{
				asset->audio_length = (samplenum)round((tocfile->toc_streams[audio_index].max_index -
					tocfile->toc_streams[audio_index].min_index) *
					av_q2d(context->streams[audio_index]->time_base) *
					asset->sample_rate);
			}
		}
		else
		{
			avlibs_lock->unlock();
			return 1;
		}
	}
	avlibs_lock->unlock();
	return result;
}

void FileAVlibs::close_file()
{
	avlibs_lock->lock("FileFFMPEG::close_file");

	if(context)
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
			if(swr_ctx)
				swr_free(&swr_ctx);
		}

		avformat_close_input(&context);
	}
	delete tocfile;
	tocfile = 0;
	avlibs_lock->unlock();
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
	rqpos = round(frame->get_source_pts() / av_q2d(stream->time_base)) +
		tocfile->toc_streams[video_index].min_index;

	if(rqpos != video_pos)
	{
		itm = tocfile->get_item(video_index, rqpos);

		if(rqpos < video_pos || itm->index > rqpos)
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

	while(!(error = av_read_frame(context, &pkt)))
	{
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
		frame->set_source_pts(av_frame_get_best_effort_timestamp(avvframe) * av_q2d(stream->time_base));
		frame->set_duration(av_frame_get_pkt_duration(avvframe) * av_q2d(stream->time_base));
// see tuleb ringi teha
		frame->set_frame_number((int)round(av_frame_get_best_effort_timestamp(avvframe)
			* av_q2d(stream->time_base) * av_q2d(stream->avg_frame_rate)));
	}
	avlibs_lock->unlock();

	if(error)
	{
		liberror(res, "FileAVlibs::read_frame:av_read_frame");
		return 1;
	}

	return error;
}

int FileAVlibs::read_aframe(AFrame *aframe)
{
	int error = 0;
	int got_it;
	int res = 0;
	int64_t rqpos;
	AVPacket pkt = {0};
	stream_item *itm;

	avlibs_lock->lock("FileAVlibs::read_aframe");
	AVStream *stream = context->streams[audio_index];
	AVCodecContext *decoder_context = stream->codec;

	int num_ch = decoder_context->channels;

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
		buffer_pos = 0;
	}

	if(!swr_ctx)
	{
		swr_ctx = swr_alloc_set_opts(NULL,
			AV_CH_LAYOUT_STEREO,
			AV_SAMPLE_FMT_DBLP,
			aframe->samplerate,
			AV_CH_LAYOUT_STEREO,
			decoder_context->sample_fmt,
			decoder_context->sample_rate, 0, 0);
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

	if(!avaframe)
		avaframe = av_frame_alloc();

	rqpos = round(aframe->source_pts / av_q2d(stream->time_base)) +
		tocfile->toc_streams[video_index].min_index;

	if(fill_from_buffer(rqpos, aframe))
	{
		avlibs_lock->unlock();
		return 0;
	}

	buffer_pts = rqpos;
	if(rqpos != audio_pos - audio_delay)
	{
		itm = tocfile->get_item(audio_index, rqpos);

		if(rqpos < audio_pos || itm->index < rqpos)
		{
			if((res = avformat_seek_file(context, audio_index,
				INT64_MIN, itm->offset, INT64_MAX,
				AVSEEK_FLAG_ANY)) < 0)
			{
				liberror(res, "FileAVlibs::read_aframe:avformat_seek_file");
				avlibs_lock->unlock();
				return -1;
			}
			avcodec_flush_buffers(decoder_context);
			audio_pos = itm->index;
		}
		while(audio_pos < rqpos && av_read_frame(context, &pkt) == 0)
		{
			if(pkt.stream_index == audio_index)
			{
				if((res = avcodec_decode_audio4(decoder_context,
					avaframe, &got_it, &pkt)) < 0)
				{
					liberror(res, "FileAVlibs::read_aframe:avcodec_decode_audio skip");
					avlibs_lock->unlock();
					return -1;
				}
				av_free_packet(&pkt);

				if(got_it)
				{
					int64_t duration = av_frame_get_pkt_duration(avaframe);
					audio_pos = avaframe->pkt_pts + duration;
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
	{
		avlibs_lock->unlock();
		return res;
	}

	while(!(error = av_read_frame(context, &pkt)))
	{
		if(pkt.stream_index == audio_index)
		{
			if((res = avcodec_decode_audio4(decoder_context,
				avaframe, &got_it, &pkt)) < 0)
			{
				liberror(res, "FileAVlibs::read_aframe:avcodec_decode_audio read");
				avlibs_lock->unlock();
				return -1;
			}
			av_free_packet(&pkt);

			if(got_it)
			{
				audio_pos = avaframe->pkt_pts + av_frame_get_pkt_duration(avaframe);

				if((res = fill_buffer(avaframe)) > 0)
				{
					if(buffer_pos >= aframe->fill_length())
						break;
				}
				else
				{
					error = res;
					break;
				}
			}
		}
	}

	if(error && error != AVERROR_EOF)
	{
		avlibs_lock->unlock();
		return error;
	}
	for(int j = 0; j < num_buffers; j++)
		buffer_valid[j] = 1;

	audio_delay = swr_get_delay(swr_ctx, avaframe->sample_rate);

	fill_from_buffer(rqpos, aframe);
	avlibs_lock->unlock();
	return 0;
}

int FileAVlibs::fill_buffer(AVFrame *avaframe, int insamples, int bps, int planar)
{
	int j, inpos;
	double *obp[MAXCHANNELS];
	const uint8_t *ibp[AV_NUM_DATA_POINTERS];

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

	int out_samples = swr_convert(swr_ctx, (uint8_t **)obp,
		buffer_len - buffer_pos,
		(const uint8_t **)&ibp,
		avaframe->nb_samples - insamples);

	if(out_samples > 0)
		buffer_pos += out_samples;

	return out_samples;
}

int FileAVlibs::fill_from_buffer(int64_t rqpos, AFrame *aframe)
{
	int copylen;
	int chnl = aframe->channel;
	int i;

	if(!buffer_valid[chnl] || rqpos != buffer_pts)
		return 0;

	copylen = MIN(buffer_pos, aframe->fill_length());
	aframe->set_filled(copylen);
	if(copylen < aframe->buffer_length)
		aframe->clear_buffer();
	memcpy(aframe->buffer, abuffer[chnl], copylen * sizeof(double));

	if(copylen < buffer_pos)
		memmove(abuffer[chnl], &abuffer[chnl][buffer_pos],
			(buffer_pos - copylen) * sizeof(double));
	buffer_valid[chnl] = 0;

	for(i = 0; i < num_buffers; i++)
		if(buffer_valid[i])
			break;

	if(i == num_buffers)
		buffer_pos -= copylen;
	return 1;
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
	for(int i = 0; known_formats[i].text; i++)
	{
		if(strncmp(context->iformat->name, known_formats[i].text,
				strlen(known_formats[i].text)) == 0)
			return known_formats[i].value;
	}
	return FILE_UNKNOWN;
}

void FileAVlibs::liberror(int code, const char *prefix)
{
	int len;
	char string[BCTEXTLEN];

	strcpy(string, prefix);
	strcat(string, ": ");
	len = strlen(string);
	av_strerror(code, string, BCTEXTLEN - 1 - len);
	errormsg("%s", string);
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
		if(!avvframe)
			avvframe = av_frame_alloc();
		break;
	}
	track_data.id = trx;
	track_data.data1 = 0;
	track_data.data2 = 0;
	track_data.min_index = 0;
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
						track_data.min_index = video_pos;
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
						track_data.min_index = audio_pos;
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
	printf("%*sdebug %d max_interleave_delta %lld strict_std_compliance %d\n", indent, "",
		ctx->debug, ctx->max_interleave_delta, ctx->strict_std_compliance);
	printf("%*smax_ts_probe %d avoid_negative_ts %d audio_preload %d max_chunk_duration %d\n", indent, "",
		ctx->max_ts_probe, ctx->avoid_negative_ts, ctx->audio_preload, ctx->max_chunk_duration);
	printf("%*sduration_estimation_method %d skip_initial_bytes %lld correct_ts_overflow %u\n", indent, "",
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

	sprintf(obuf, "%lld", ts);
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

const char *FileAVlibs::dump_AVRational(AVRational *r, char *obuf)
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
	printf("%*stime_base %s start_time %s duration %s frames %lld\n", indent, "",
		dump_AVRational(&stm->time_base), dump_ts(stm->start_time, bf1),
		dump_ts(stm->duration, bf2), stm->nb_frames);
	printf("%*sdisposition %#x discard %d, sample_aspect_ratio %s\n", indent, "",
		stm->disposition, stm->discard, dump_AVRational(&stm->sample_aspect_ratio));
	printf("%*smetadata %p avg_frame_rate %s side_data %p nb_side_data %d\n", indent, "",
		stm->metadata, dump_AVRational(&stm->avg_frame_rate),
		stm->side_data, stm->nb_side_data);
// FIXIT Attached_pic dump here!
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
	printf("%*sskip_to_keyframe %d skip_samples %d start_skip_samples %lld first_discard_sample %lld\n", indent, "",
		stm->skip_to_keyframe, stm->skip_samples, stm->start_skip_samples,
		stm->first_discard_sample);
	printf("%*slast_discard_sample %lld nb_decoded_frames %d mux_ts_offset %lld\n", indent, "",
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
	printf("%*scodec_id %d fourcc %s priv_data %p, internal %p\n", indent, "",
		ctx->codec_id, dump_fourcc(ctx->codec_tag), ctx->priv_data, ctx->internal);
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
	printf("%*schannel_layout %#llx request_channel_layout %#llx audio_service_type %d\n", indent, "",
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
	printf("%*stimecode_frame_start %lld rtp_callback %p rtp_payload_size %d\n", indent, "",
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
	printf("%*ssubtitle_header %p subtitle_header_size %d vbv_delay %lld\n", indent, "",
		ctx->subtitle_header, ctx->subtitle_header_size, ctx->vbv_delay);
	printf("%*sside_data_only_packets %d initial_padding %d framerate %s\n", indent, "",
		ctx->side_data_only_packets, ctx->initial_padding, dump_AVRational(&ctx->framerate));
	printf("%*ssw_pix_fmt %d pkt_timebase %s codec_descriptor %p\n", indent, "",
		ctx->sw_pix_fmt, dump_AVRational(&ctx->pkt_timebase), ctx->codec_descriptor);
	printf("%*spts_correction_num_faulty_pts %lld pts_correction_num_faulty_dts %lld\n", indent, "",
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
	printf("%*schannel_layout %#llx extended_buf %p nb_extended_buf %d\n", indent, "",
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
