
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
#include <string.h>

struct selection_int FileAVlibs::known_formats[] =
{
	{ "mov,mp4,m4a,3gp,3g2,mj2", FILE_MOV },
// avi does not work yet
//	{ "avi", FILE_AVI },
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
				asset->audio_length = tocfile->toc_streams[audio_index].max_index -
						tocfile->toc_streams[audio_index].min_index;
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
