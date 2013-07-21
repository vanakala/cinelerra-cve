
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "aframe.h"
#include "bcsignals.h"
#include "byteorder.h"
#include "clip.h"
#include "condition.h"
#include "edit.h"
#include "file.h"
#include "fileogg.h"
#include "filetoc.h"
#include "guicast.h"
#include "language.h"
#include "mutex.h"
#include "quicktime.h"
#include "vframe.h"
#include "videodevice.inc"
#include "cmodel_permutation.h"
#include "interlacemodes.h"
#include "mainerror.h"
#include "theme.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Needed for packaging engine
#include "preferences.h"
#include "render.h"

extern Theme *theme_global;

#define READ_SIZE 66000


FileOGG::FileOGG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_OGG;

	asset->byte_order = 0;
	free_stream = 0;
	open_streams = 0;
	configured_streams = 0;
	temp_frame = 0;
	stream = 0;
	flush_lock = 0;
	tocfile = 0;
	page_write_error = 0;
	reading = 0;
	writing = 0;
	memset(&track_data, 0, sizeof(track_data));
}

FileOGG::~FileOGG()
{
	if(stream) close_file();
}

void FileOGG::get_parameters(BC_WindowBase *parent_window,
	Asset *asset,
	BC_WindowBase* &format_window,
	int options)
{
	if(options & SUPPORTS_AUDIO)
	{
		OGGConfigAudio *window = new OGGConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(options & SUPPORTS_VIDEO)
	{
		OGGConfigVideo *window = new OGGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileOGG::read_buffer()
{
	int rlen;
	char *buffer = ogg_sync_buffer(&sync_state, READ_SIZE);

	rlen = fread(buffer, 1, READ_SIZE, stream);
	ogg_sync_wrote(&sync_state, rlen);
	file_bufpos += rlen;
	return rlen;
}

int FileOGG::read_buffer_at(off_t filepos)
{
	fseeko(stream, filepos, SEEK_SET);
	file_bufpos = filepos;
	file_pagepos = filepos; // this one is not valid until sync_pageseek!
	ogg_sync_reset(&sync_state);
// Invalidate current frame / sample position
	sample_position = -1;
	frame_position = -1;
	return read_buffer();
}

int FileOGG::next_oggpage(ogg_page *op)
{
	while(ogg_sync_pageout(&sync_state, op) != 1)
	{
		if(read_buffer() <= 0)
			return 1;
	}
	file_pagestart = file_pagepos;
	file_pagepos += op->header_len + op->body_len;
	return 0;
}

int FileOGG::next_oggstreampage(ogg_page *op)
{
	int rv;

	while((rv = next_oggpage(op)) == 0)
	{
		if(cur_stream->state.serialno == ogg_page_serialno(op))
			return 0;
	}
	return rv;
}

int FileOGG::sync_page(ogg_page *page)
{
	int ret;

	while((ret = ogg_sync_pageseek(&sync_state, page)) == 0)
	{
		if(read_buffer() <= 0)
			return 0;
	}
	if(ret < 0)
	{
		file_pagepos -= ret;
	}
	else if(ret > 0)
	{
		file_pagepos += ret;
	}
	return ret;
}

void FileOGG::set_audio_stream()
{
	for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
		if(cur_stream->vi)
			return;
	cur_stream = 0;
	errormsg("No current OGG audio stream");
}

void FileOGG::set_video_stream(VFrame *frame)
{
	for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
		if(cur_stream->ti)
			return;
	cur_stream = 0;
	errormsg("No current OGG video stream");
}

int FileOGG::open_file(int rd, int wr)
{
	int ret = 0;

	free_stream = 0;
	reading = rd;
	writing = wr;

	memset(streams, 0, sizeof(streams));

	if(wr)
	{

		if((stream = fopen(asset->path, "w+b")) == 0)
		{
			errormsg("Error while opening \"%s\" for writing. %m", asset->path);
			return 1;
		}

		// yayness.  Set up Ogg output stream
		srand (time (NULL));

		if(asset->video_data)
		{
			cur_stream = &streams[free_stream++];
			ogg_stream_init(&cur_stream->state, rand());
			cur_stream->ti = new theora_info;
			theora_info_init(cur_stream->ti);

			cur_stream->ti->frame_width = asset->width; 
			cur_stream->ti->frame_height = asset->height;

			// round width & height up to the nearest multiple of 16
			cur_stream->ti->width = (asset->width + 15) & ~0xf;
			cur_stream->ti->height = (asset->height + 15) & ~0xf;
			// center the frame
			cur_stream->ti->offset_x = (cur_stream->ti->width - cur_stream->ti->frame_width) / 2;
			cur_stream->ti->offset_y = (cur_stream->ti->height - cur_stream->ti->frame_height) / 2;
			cur_stream->ti->fps_numerator = (unsigned int)(asset->frame_rate * 1000000);
			cur_stream->ti->fps_denominator = 1000000;

			if(asset->aspect_ratio > 0)
			{
				// Cinelerra uses frame aspect ratio, theora uses pixel aspect ratio
				float pixel_aspect = asset->aspect_ratio / asset->width * asset->height;
				cur_stream->ti->aspect_numerator = (unsigned int)(pixel_aspect * 1000000);
				cur_stream->ti->aspect_denominator = 1000000;
			}
			else
			{
				cur_stream->ti->aspect_numerator = 1000000;
				cur_stream->ti->aspect_denominator = 1000000;
			}
			if(EQUIV(asset->frame_rate, 25) || EQUIV(asset->frame_rate, 50))
				cur_stream->ti->colorspace = OC_CS_ITU_REC_470BG;
			else if((asset->frame_rate > 29 && asset->frame_rate < 31) || (asset->frame_rate > 59 && asset->frame_rate < 61))
				cur_stream->ti->colorspace = OC_CS_ITU_REC_470M;
			else
				cur_stream->ti->colorspace = OC_CS_UNSPECIFIED;

			if(asset->theora_fix_bitrate)
			{
				cur_stream->ti->target_bitrate = asset->theora_bitrate; 
				cur_stream->ti->quality = 0;
			}
			else
			{
				cur_stream->ti->target_bitrate = 0;
				cur_stream->ti->quality = asset->theora_quality;     // video quality 0-63
			}
			cur_stream->ti->dropframes_p = 0;
			cur_stream->ti->quick_p = 1;
			cur_stream->ti->keyframe_auto_p = 1;
			cur_stream->ti->keyframe_frequency = asset->theora_keyframe_frequency;
			cur_stream->ti->keyframe_frequency_force = asset->theora_keyframe_force_frequency;
			cur_stream->ti->keyframe_data_target_bitrate = (unsigned int) (cur_stream->ti->target_bitrate * 1.5);
			cur_stream->ti->keyframe_auto_threshold = 80;
			cur_stream->ti->keyframe_mindistance = 8;
			cur_stream->ti->noise_sensitivity = 1;
			cur_stream->ti->sharpness = asset->theora_sharpness; // 0, 1, 2

			if(theora_encode_init(&cur_stream->ts, cur_stream->ti))
			{
				errormsg("Initialization of theora codec failed");
				return 1;
			}
		}
		// init theora done

		// initialize Vorbis too, if we have audio.
		if(asset->audio_data)
		{
			cur_stream = &streams[free_stream++];

			ogg_stream_init(&cur_stream->state, rand());
			cur_stream->vi = new vorbis_info;
			vorbis_info_init(cur_stream->vi);
			int ret;
			if(!asset->vorbis_vbr) 
			{
				// Encoding using a VBR quality mode.
				ret = vorbis_encode_init(cur_stream->vi,
							asset->channels,
							asset->sample_rate,
							asset->vorbis_max_bitrate,
							asset->vorbis_bitrate,
							asset->vorbis_min_bitrate);
			}
			else
			{
				// Set true VBR as demonstrated by http://svn.xiph.org/trunk/vorbis/examples/encoding_example.c
				ret = vorbis_encode_setup_managed(cur_stream->vi,
					asset->channels,
					asset->sample_rate,
					-1,
					asset->vorbis_bitrate,
					-1);
				ret |= vorbis_encode_ctl(cur_stream->vi, OV_ECTL_RATEMANAGE_AVG, NULL);
				ret |= vorbis_encode_setup_init(cur_stream->vi);
			}

			if(ret)
			{
				errormsg("The Vorbis encoder could not set up a mode according to"
						" the requested quality or bitrate.");
				return 1;
			}
			// set up the analysis state and auxiliary encoding storage
			vorbis_analysis_init(&cur_stream->vs, cur_stream->vi);
			vorbis_block_init(&cur_stream->vs, &cur_stream->block);
		}
		// audio init done

		// write the bitstream header packets with proper page interleave
		// first packet will get its own page automatically
		for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
		{
			if(cur_stream->ti)
			{
				theora_encode_header(&cur_stream->ts, &pkt);
				ogg_stream_packetin(&cur_stream->state, &pkt);
				if(ogg_stream_pageout(&cur_stream->state, &page) != 1)
				{
					errormsg("Failed to encode OGG Theora header");
					return 1;
				}
				if(write_page(&page))
				{
					errormsg("Failed to write Ogg Theora first header");
					return 1;
				}

				// create the remaining theora headers
				theora_comment_init(&the_comment);
				theora_comment_add_tag(&the_comment, (char*)"ENCODER", (char*)PACKAGE_STRING);
				theora_encode_comment(&the_comment, &pkt);
				ogg_stream_packetin (&cur_stream->state, &pkt);
				theora_comment_clear(&the_comment);
				theora_encode_tables (&cur_stream->ts, &pkt);
				ogg_stream_packetin(&cur_stream->state, &pkt);
			}
			if(cur_stream->vi)
			{
				ogg_packet header_comm;
				ogg_packet header_code;

				vorbis_comment_init(&vrb_comment);
				vorbis_comment_add_tag(&vrb_comment, (char*)"ENCODER", (char*)PACKAGE_STRING);
				vorbis_analysis_headerout (&cur_stream->vs,
					&vrb_comment, &pkt, &header_comm, &header_code);
				vorbis_comment_clear(&vrb_comment);
				ogg_stream_packetin(&cur_stream->state, &pkt);
				if(ogg_stream_pageout(&cur_stream->state, &page) != 1)
				{
					errormsg("Failed to encode Ogg Vorbis first header");
					return 1;
				}
				if(write_page(&page))
				{
					errormsg("Failed to write Ogg Vorbis first header");
					return 1;
				}

				// remaining vorbis header packets
				ogg_stream_packetin(&cur_stream->state, &header_comm);
				ogg_stream_packetin(&cur_stream->state, &header_code);
			}
		}
		page_write_error = 0;
		// Flush the rest of our headers. This ensures
		// the actual data in each stream will start
		// on a new page, as per spec.
		for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
		{
			if(cur_stream->ti || cur_stream->vi)
			{
				int ret;
				while(ret = ogg_stream_flush(&cur_stream->state, &page))
				{
					if(write_page(&page))
					{
						errormsg("Failed to write OGG headers");
						return 1;
					}
				}
				cur_stream->page_valid = 0;
			}
		}
		flush_lock = new Mutex("OGGFile::Flush lock");
	}
	else if(rd)
	{
		if((stream = fopen(asset->path, "rb")) == 0)
		{
			errormsg("Error while opening %s for reading. %m", asset->path);
			return 1;
		}
		// get file length
		struct stat file_stat;
		stat(asset->path, &file_stat);
		asset->file_length = file_stat.st_size;

		// Check existence of toc
		tocfile = new FileTOC(this, file->preferences->index_directory, asset->path,
			asset->file_length, file_stat.st_mtime);
		ogg_sync_init(&sync_state);

		// make sure we init the position structures to zero
		read_buffer_at(0);

		do
		{
			if(next_oggpage(&page))
			{
				errorbox("Can't find ogg headers in %s", asset->path);
				fclose(stream);
				return 1;
			}
			if(ogg_page_bos(&page) && (ogg_stream_init(&streams[free_stream].state,
					ogg_page_serialno(&page)) == 0))
			{
				cur_stream = &streams[free_stream++];
				open_streams++;
			}
			else
			{
				int s = ogg_page_serialno(&page);
				for(cur_stream = streams; cur_stream < & streams[free_stream]; cur_stream++)
				{
					if(cur_stream->state.serialno == s)
						break;
				}
			}
			if(ogg_page_eos(&page))
			{
				cur_stream->headers = 0;
				cur_stream->dec_init = 0;
				open_streams--;
			}
			ogg_stream_pagein(&cur_stream->state, &page);
			while(ogg_stream_packetout(&cur_stream->state, &pkt) == 1)
			{
				if(cur_stream->headers < 3)
				{
					if(cur_stream->headers == 0)
					{
						vorbis_info_init(&vrb_info);
						vorbis_comment_init(&vrb_comment);
						theora_comment_init(&the_comment);
						theora_info_init(&the_info);
						if(theora_decode_header(&the_info, &the_comment, &pkt) == 0)
						{
							cur_stream->ti = new theora_info;
							*cur_stream->ti = the_info;
							cur_stream->tc = new theora_comment;
							*cur_stream->tc = the_comment;
						}
						else if(vorbis_synthesis_headerin(&vrb_info, &vrb_comment, &pkt) == 0)
						{
							cur_stream->vi = new vorbis_info;
							*cur_stream->vi = vrb_info;
							cur_stream->vc = new vorbis_comment;
							*cur_stream->vc = vrb_comment;
						}
						cur_stream->headers++;
					}
					else
					{
						if(cur_stream->ti)
							if(theora_decode_header(cur_stream->ti, cur_stream->tc, &pkt))
							{
								errorbox("Failed to decode theora header");
								return 1;
							}
						if(cur_stream->vi)
							if(vorbis_synthesis_headerin(cur_stream->vi, cur_stream->vc, &pkt))
							{
								errorbox("Failed to decode vorbis header");
								return 1;
							}
						cur_stream->headers++;
					}
					if(cur_stream->headers == 3)
						configured_streams++;
				}
			}
		} while(configured_streams < open_streams);

		// Remember where the real data begins for later seeking purposes
		filedata_begin = file_pagepos;

		// and now we have it all.  initialize decoders
		for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
		{
			if(cur_stream->ti)
			{
				ret |= theora_decode_init(&cur_stream->ts, cur_stream->ti);
				double fps = (double)cur_stream->ti->fps_numerator / cur_stream->ti->fps_denominator;
				theora_cmodel = BC_YUV420P;

				// FIXME - LOW PRIORITY Cinelerra does not honor the frame_width and frame_height
				asset->width = cur_stream->ti->frame_width;
				asset->height = cur_stream->ti->frame_height;

// Don't want a user configured frame rate to get destroyed
				if(!asset->frame_rate)
					asset->frame_rate = fps;
// All theora material is noninterlaced by definition
				if(!asset->interlace_mode)
					asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

				asset->video_data = 1;
				asset->layers = 1;
				double pixaspect = 1;
				if(cur_stream->ti->aspect_denominator)
					pixaspect = (double)cur_stream->ti->aspect_numerator / cur_stream->ti->aspect_denominator;
				asset->aspect_ratio = 
					(double)cur_stream->ti->frame_width / cur_stream->ti->frame_height
					* pixaspect;
				strncpy(asset->vcodec, "theo", 4);
			}
			if(cur_stream->vi)
			{
				ret |= vorbis_synthesis_init(&cur_stream->vs, cur_stream->vi);
				vorbis_block_init(&cur_stream->vs, &cur_stream->block);

				asset->channels = cur_stream->vi->channels;
				if(asset->channels > MAXCHANNELS)
				{
					errorbox("Too many channels in ogg audio stream (max %d)", MAXCHANNELS);
					return 1;
				}
				if(!asset->sample_rate)
					asset->sample_rate = cur_stream->vi->rate;
				asset->audio_data = 1;

				strncpy(asset->acodec, "vorb", 4);
			}

		if(ret)
			break;
		}
	}

	if(!ret && rd)
	{
		// Everything is in OGG container
		if(!(ret = tocfile->init_tocfile(TOCFILE_TYPE_MUX0)))
		{
			for(cur_stream = streams; cur_stream < & streams[free_stream]; cur_stream++)
			{
				int k = cur_stream->toc_streamno = tocfile->id_to_index(cur_stream->state.serialno);

				if(cur_stream->vi)
					asset->audio_length = tocfile->toc_streams[k].max_index;
				if(cur_stream->ti)
					asset->video_length = tocfile->toc_streams[k].max_index;
			}
		}
	}
	return ret;
}

int FileOGG::check_sig(Asset *asset)
{
	FILE *fd = fopen(asset->path, "rb");

// Test for "OggS"
	char data[4];
	if(fread(data, 4, 1, fd) < 1)
	{
		fclose(fd);
		return 0;
	}
	if(data[0] == 'O' &&
		data[1] == 'g' &&
		data[2] == 'g' &&
		data[3] == 'S')
	{
		fclose(fd);
		return 1;
	}

	fclose(fd);

	return 0;
}

int FileOGG::supports(int format)
{
	if(format == FILE_OGG)
		return SUPPORTS_AUDIO | SUPPORTS_VIDEO;
	return 0;
}

void FileOGG::close_file()
{
	if(!stream)
		return;

	if(writing)
	{
		flush_ogg(1); // flush all

		if(flush_lock) delete flush_lock;
		flush_lock = 0;

		if(temp_frame)
		{
			delete temp_frame;
			temp_frame = 0;
		}
	}
	for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
	{
		if(cur_stream->ti)
		{
			theora_info_clear(cur_stream->ti);
			theora_comment_clear(cur_stream->tc);
			theora_clear(&cur_stream->ts);
			delete cur_stream->ti;
			if(reading)
				delete cur_stream->tc;
			cur_stream->ti = 0;
		}
		else if(cur_stream->vi)
		{
			vorbis_block_clear(&cur_stream->block);
			vorbis_dsp_clear(&cur_stream->vs);
			vorbis_comment_clear(cur_stream->vc);
			vorbis_info_clear(cur_stream->vi);
			if(cur_stream->max_pcm_samples)
			{
				for(int i = 0; i < cur_stream->max_pcm_samples; i++)
					delete cur_stream->pcm_samples[i];
				cur_stream->max_pcm_samples = 0;
			}
			delete cur_stream->vi;
			if(reading)
				delete cur_stream->vc;
			cur_stream->vi = 0;
		}
		ogg_stream_clear(&cur_stream->state);
		cur_stream->headers = 0;
		cur_stream->dec_init = 0;
	}
	if(reading)
	{
		if(tocfile) delete tocfile;
		ogg_sync_clear(&sync_state);
	}
	if(stream) page_write_error |= fclose(stream);
	if(writing && page_write_error)
		errormsg("Failed to write OGG file to disk");
	stream = 0;
}

int FileOGG::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileOGG::get_best_colormodel(Asset *asset, int driver)
{
	return BC_YUV420P;
}

int FileOGG::read_frame(VFrame *frame)
{
	framenum next_frame_position;
	yuv_buffer yuv;
	int rv;
	stream_item *itm;

	if(!stream) return 1;

	next_frame_position = (frame->get_source_pts() + FRAME_OVERLAP) * asset->frame_rate;

	if(next_frame_position < 0)
		return 1;
	for(cur_stream = streams; cur_stream < & streams[free_stream]; cur_stream++)
		if(cur_stream->ti)
			break;

	// skip is cheaper than seek, do it...
	if(next_frame_position != frame_position)
	{
		int length;

		if((itm = tocfile->get_item(cur_stream->toc_streamno, next_frame_position)) == 0)
			return 1;

		if(frame_position < 0 || next_frame_position < frame_position 
			|| itm->index > frame_position)
		{
			read_buffer_at(itm->offset);
			if(next_oggstreampage(&page))
				return 1;
			rv = ogg_stream_reset(&cur_stream->state);

			frame_position = -1;
			ogg_stream_pagein(&cur_stream->state, &page);
		}
	}

	while(frame_position <= next_frame_position)
	{
		while(ogg_stream_packetout(&cur_stream->state, &pkt) != 1){
			if(next_oggstreampage(&page))
				return 1;
			ogg_stream_pagein(&cur_stream->state, &page);
		}

		// decode
		theora_decode_packetin(&cur_stream->ts, &pkt);
		if(frame_position < 0)
		{
			if(theora_packet_iskeyframe(&pkt))
				frame_position = itm->index;
		}
		else
			frame_position++;
	}

	if(theora_decode_YUVout(&cur_stream->ts, &yuv))
		return 1;

	cmodel_transfer(frame->get_rows(),
		0,
		frame->get_y(),
		frame->get_u(),
		frame->get_v(),
		yuv.y,
		yuv.u,
		yuv.v,
		cur_stream->ti->offset_x,
		cur_stream->ti->offset_y,
		cur_stream->ti->frame_width,
		cur_stream->ti->frame_height,
		0,
		0,
		frame->get_w(),  // temp_frame can be larger than frame if width not dividable by 16
		frame->get_h(),
		BC_YUV420P,
		frame->get_color_model(),
		0,
		yuv.y_stride,
		frame->get_w());

	next_frame_position = frame_position - 1;
	frame->set_source_pts((ptstime)next_frame_position / asset->frame_rate);
	frame->set_duration(1. / asset->frame_rate);
	frame->set_frame_number(next_frame_position);
	return 0;
}

void FileOGG::move_pcmsamples_position()
{
	if(next_sample_position >= cur_stream->pcm_start &&
			next_sample_position < cur_stream->pcm_start + cur_stream->pcm_size)
	{
		int from = next_sample_position - cur_stream->pcm_start;
		int len = cur_stream->pcm_size - from;
		for(int i = 0; i < asset->channels; i++)
			memmove(cur_stream->pcm_samples[i], cur_stream->pcm_samples[i] + from, sizeof(float) * len);
		cur_stream->pcm_size = len;
		cur_stream->pcm_start = next_sample_position;
	}
}

int FileOGG::copy_to_pcmsamples(float **smpls, int start, int smplen)
{
	int cplen = smplen - start;

	if(cplen + cur_stream->pcm_size > cur_stream->pcm_max)
		cplen = cur_stream->pcm_max - cur_stream->pcm_size;

	if(cplen > 0)
	{
		for(int i = 0; i < asset->channels; i++)
			memcpy(cur_stream->pcm_samples[i] + cur_stream->pcm_size, smpls[i] + start, cplen * sizeof(float));

		if(cur_stream->pcm_size == 0)
			cur_stream->pcm_start = sample_position + start;

		cur_stream->pcm_size += cplen;
		return cplen;
	}
	return 0;
}

int FileOGG::fill_pcm_samples(int len)
{
	stream_item *itm;
	float **pcmbuf;
	int num;
	samplenum pos = next_sample_position;

	if(cur_stream->pcm_start >= 0 && next_sample_position >= cur_stream->pcm_start && 
		next_sample_position < cur_stream->pcm_start + cur_stream->pcm_size)
	{
		if(next_sample_position > cur_stream->pcm_start)
			move_pcmsamples_position();
		if(cur_stream->pcm_size >= len)
			return 0;
		pos = cur_stream->pcm_start + cur_stream->pcm_size;
	}

	if(pos != sample_position)
	{
		// We have to seek
		if((itm = tocfile->get_item(cur_stream->toc_streamno, pos)) == 0)
			return 1;
		if(pos < 0 || pos < sample_position
			|| itm->index > sample_position)
		{
			read_buffer_at(itm->offset);
			if(next_oggstreampage(&page))
				return 1;
			ogg_stream_reset(&cur_stream->state);
			sample_position = -1;
			ogg_stream_pagein(&cur_stream->state, &page);
			vorbis_synthesis_restart(&cur_stream->vs);
			cur_stream->pcm_size = 0;
		}
	}
	while(sample_position <= pos)
	{
		while(ogg_stream_packetout(&cur_stream->state, &pkt) != 1)
		{
			if(next_oggstreampage(&page))
				return 1;
			ogg_stream_pagein(&cur_stream->state, &page);
		}
		if(vorbis_synthesis(&cur_stream->block, &pkt) ||
			vorbis_synthesis_blockin(&cur_stream->vs, &cur_stream->block))
		{
			errorbox("Failed to feed packet to vorbis decoder");
			return 1;
		}
		num = vorbis_synthesis_pcmout(&cur_stream->vs, &pcmbuf);

		if(num > 0)
		{
			if(sample_position < 0)
				sample_position = itm->index;

			if(sample_position + num > pos)
				num = copy_to_pcmsamples(pcmbuf, pos - sample_position, num)
					+ pos - sample_position;
		}
		vorbis_synthesis_read(&cur_stream->vs, num);
		sample_position += num;
	}

	len = MIN(len, cur_stream->pcm_max - cur_stream->pcm_size);

	while(cur_stream->pcm_size < len)
	{
		while(ogg_stream_packetout(&cur_stream->state, &pkt) != 1)
		{
			if(next_oggstreampage(&page))
				return 1;
			ogg_stream_pagein(&cur_stream->state, &page);
		}
		if(vorbis_synthesis(&cur_stream->block, &pkt) ||
			vorbis_synthesis_blockin(&cur_stream->vs, &cur_stream->block))
		{
			errorbox("Failed to feed packet to vorbis decoder");
			return 1;
		}
		num = vorbis_synthesis_pcmout(&cur_stream->vs, &pcmbuf);
		num = copy_to_pcmsamples(pcmbuf, 0, num);
		vorbis_synthesis_read(&cur_stream->vs, num);
		sample_position += num;
	}
	return 0;
}

int FileOGG::read_aframe(AFrame *aframe)
{
	float **vorbis_buffer;
	int filled;
	double *buffer = 0;
	float *float_buffer = 0;

	next_sample_position = aframe->position;
	int len = aframe->source_length;

	if(len <= 0)
		return 0;

	for(cur_stream = streams; cur_stream < & streams[free_stream]; cur_stream++)
		if(cur_stream->vi)
			break;

	if(!cur_stream->max_pcm_samples)
	{
		for(int i = 0; i < asset->channels; i++)
			cur_stream->pcm_samples[i] = new float[MAX_AUDIO_BUFFER_SIZE];
		cur_stream->pcm_start = -100000000; // insane value to trigger reload
		cur_stream->pcm_size = 0;
		cur_stream->max_pcm_samples = asset->channels;
		cur_stream->pcm_max = MAX_AUDIO_BUFFER_SIZE;
	}

	filled = 0;
	if(aframe->buffer)
		buffer = &aframe->buffer[aframe->length];
	else
		float_buffer = &aframe->float_buffer[aframe->length];

	while(filled < len)
	{
		if(fill_pcm_samples(len))
			return 0;

		float *input = &cur_stream->pcm_samples[aframe->channel][filled];
		int chunk = MIN(cur_stream->pcm_size, len - filled);

		if(buffer)
		{
			for (int i = 0; i < chunk; i++)
				buffer[i] = input[i];
		}
		else
		{
			for (int i = 0; i < chunk; i++)
				float_buffer[i] = input[i];
		}

		filled += chunk;
		next_sample_position += chunk;
	}
	aframe->set_filled_length();
	return 0;
}

int FileOGG::prefer_samples_float()
{
	return 1;
}

int FileOGG::write_page(ogg_page *op)
{
	if(fwrite(op->header, 1, op->header_len, stream) != op->header_len ||
			fwrite(op->body, 1, op->body_len, stream) != op->body_len)
	{
		page_write_error = 1;
		return 1;
	}
	return 0;
}

int FileOGG::write_file()
{
	ptstime time_min = 1000000;
	media_stream_t *min_stream, *strm;

	min_stream = 0;

	for(strm = streams; strm < &streams[free_stream]; strm++)
	{
		if(strm->page_valid && strm->page_pts < time_min)
		{
			time_min = strm->page_pts;
			min_stream = strm;
		}
	}
	if(min_stream)
	{
		write_page(&min_stream->page);
		min_stream->page_valid = 0;
		return 0;
	}
	return 1;
}

void FileOGG::flush_ogg(int eos)
{
	media_stream_t *cur_stream;

	flush_lock->lock("FileOGG::flush_ogg");

	for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
	{
		if(cur_stream->ti)
		{
			while(theora_encode_packetout(&cur_stream->ts, eos, &pkt))
				ogg_stream_packetin(&cur_stream->state, &pkt);
		}
		if(cur_stream->vi)
		{
			if(eos)
				vorbis_analysis_wrote(&cur_stream->vs, 0);

			while(vorbis_analysis_blockout(&cur_stream->vs, &cur_stream->block) == 1)
			{
				vorbis_analysis(&cur_stream->block, NULL);
				vorbis_bitrate_addblock(&cur_stream->block);

				while(vorbis_bitrate_flushpacket(&cur_stream->vs, &pkt) == 1)
					ogg_stream_packetin(&cur_stream->state, &pkt);
			}
		}
	}

	do
	{
		for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
		{
			if(cur_stream->ti || cur_stream->vi)
			{
				if(!cur_stream->page_valid &&
					ogg_stream_flush(&cur_stream->state, &cur_stream->page))
				{
					if(cur_stream->ti)
						cur_stream->page_pts = theora_granule_time(&cur_stream->ts, ogg_page_granulepos(&cur_stream->page));
					else
						cur_stream->page_pts = vorbis_granule_time(&cur_stream->vs, ogg_page_granulepos(&cur_stream->page));
					cur_stream->page_valid = 1;
				}
			}
		}
	}
	while(write_file() == 0);
	flush_lock->unlock();

	if(page_write_error)
		errormsg("Failed to write rendered OGG");
}

int FileOGG::write_aframes(AFrame **frames)
{
	int i, j, len;
	float **vrb_buf;

	len = frames[0]->length;

	if(len > 0)
	{
		set_audio_stream();
		flush_lock->lock("FileOGG::write_samples");
		vrb_buf = vorbis_analysis_buffer(&cur_stream->vs, len);

		for(i = 0; i < asset->channels; i++)
		{
			double *buffer = frames[i]->buffer;
			len = frames[i]->length;

			for(j = 0; j < len; j++)
				vrb_buf[i][j] = buffer[j];
		}
		vorbis_analysis_wrote(&cur_stream->vs, len);
		flush_lock->unlock();

		flush_ogg(0);
	}

	return page_write_error;
}

int FileOGG::write_frames(VFrame ***frames, int len)
{
	yuv_buffer yuv;
	int j, ret;
	int w, h, b;

	flush_ogg(0);

	for(j = 0; j < len; j++)
	{
		VFrame *frame = frames[0][j];

		set_video_stream(frame);

		if(!temp_frame)
		{
			temp_frame = new VFrame (0,
				cur_stream->ti->width,
				cur_stream->ti->height,
				BC_YUV420P);
		}
// Always convert: internally YUV420P is never used
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
			cur_stream->ti->offset_x,
			cur_stream->ti->offset_y,
			temp_frame->get_w(),
			temp_frame->get_h(),
			frame->get_color_model(),
			BC_YUV420P,
			0,
			frame->get_w(),
			temp_frame->get_w());

		h = temp_frame->get_h();
		w = temp_frame->get_w();
		b = temp_frame->get_bytes_per_line();

		yuv.y = temp_frame->get_y();
		yuv.u = temp_frame->get_u();
		yuv.v = temp_frame->get_v();

		yuv.y_width = w;
		yuv.y_height = h;
		yuv.y_stride = b;

		yuv.uv_width = w / 2;
		yuv.uv_height = h / 2;
		yuv.uv_stride = b /2;

		flush_lock->lock("FileOGG::write_frames");
		ret = theora_encode_YUVin(&cur_stream->ts, &yuv);
		flush_lock->unlock();

		if(ret)
		{
			errormsg("Theora_encode_YUVin() failed with code %i yuv_buffer: y_width: %i, y_height: %i, y_stride: %i, uv_width: %i, uv_height: %i, uv_stride: %i",
				ret,
				yuv.y_width,
				yuv.y_height,
				yuv.y_stride,
				yuv.uv_width,
				yuv.uv_height,
				yuv.uv_stride);
			return 1;
		}
		if(j < len - 1)
			flush_ogg(0);
	}

	return page_write_error;
}


OGGConfigAudio::OGGConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	350,
	250)
{
	set_icon(theme_global->get_image("mwindow_icon"));
	this->asset = asset;
}

OGGConfigAudio::~OGGConfigAudio()
{
}

void OGGConfigAudio::create_objects()
{
	int x = 10, y = 10;
	int x1 = 150;
	char string[BCTEXTLEN];

	add_tool(fixed_bitrate = new OGGVorbisFixedBitrate(x, y, this));
	add_tool(variable_bitrate = new OGGVorbisVariableBitrate(x1, y, this));

	y += 30;
	sprintf(string, "%d", asset->vorbis_min_bitrate);
	add_tool(new BC_Title(x, y, _("Min bitrate:")));
	add_tool(new OGGVorbisMinBitrate(x1, y, this, string));

	y += 30;
	add_tool(new BC_Title(x, y, _("Avg bitrate:")));
	sprintf(string, "%d", asset->vorbis_bitrate);
	add_tool(new OGGVorbisAvgBitrate(x1, y, this, string));

	y += 30;
	add_tool(new BC_Title(x, y, _("Max bitrate:")));
	sprintf(string, "%d", asset->vorbis_max_bitrate);
	add_tool(new OGGVorbisMaxBitrate(x1, y, this, string));

	add_subwindow(new BC_OKButton(this));
	show_window();
	flush();
}

void OGGConfigAudio::close_event()
{
	set_done(0);
}


OGGVorbisFixedBitrate::OGGVorbisFixedBitrate(int x, int y, OGGConfigAudio *gui)
 : BC_Radial(x, y, !gui->asset->vorbis_vbr, _("Average bitrate"))
{
	this->gui = gui;
}

int OGGVorbisFixedBitrate::handle_event()
{
	gui->asset->vorbis_vbr = 0;
	gui->variable_bitrate->update(0);
	return 1;
}


OGGVorbisVariableBitrate::OGGVorbisVariableBitrate(int x, int y, OGGConfigAudio *gui)
 : BC_Radial(x, y, gui->asset->vorbis_vbr, _("Variable bitrate"))
{
	this->gui = gui;
}

int OGGVorbisVariableBitrate::handle_event()
{
	gui->asset->vorbis_vbr = 1;
	gui->fixed_bitrate->update(0);
	return 1;
}


OGGVorbisMinBitrate::OGGVorbisMinBitrate(int x, 
	int y, 
	OGGConfigAudio *gui, 
	char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}

int OGGVorbisMinBitrate::handle_event()
{
	gui->asset->vorbis_min_bitrate = atol(get_text());
	return 1;
}


OGGVorbisMaxBitrate::OGGVorbisMaxBitrate(int x, 
	int y, 
	OGGConfigAudio *gui,
	char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}
int OGGVorbisMaxBitrate::handle_event()
{
	gui->asset->vorbis_max_bitrate = atol(get_text());
	return 1;
}


OGGVorbisAvgBitrate::OGGVorbisAvgBitrate(int x, int y, OGGConfigAudio *gui, char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}

int OGGVorbisAvgBitrate::handle_event()
{
	gui->asset->vorbis_bitrate = atol(get_text());
	return 1;
}


OGGConfigVideo::OGGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	450,
	220)
{
	set_icon(theme_global->get_image("mwindow_icon"));
	this->asset = asset;
}

OGGConfigVideo::~OGGConfigVideo()
{
}

void OGGConfigVideo::create_objects()
{
	int x = 10, y = 10;
	int x1 = x + 150;
	int x2 = x + 300;

	add_subwindow(new BC_Title(x, y + 5, _("Bitrate:")));
	add_subwindow(new OGGTheoraBitrate(x1, y, this));
	add_subwindow(fixed_bitrate = new OGGTheoraFixedBitrate(x2, y, this));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Quality:")));
	add_subwindow(new BC_ISlider(x + 80, 
		y,
		0,
		200,
		200,
		0,
		63,
		asset->theora_quality,
		0,
		0,
		&asset->theora_quality));

	add_subwindow(fixed_quality = new OGGTheoraFixedQuality(x2, y, this));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Keyframe frequency:")));
	OGGTheoraKeyframeFrequency *keyframe_frequency = 
		new OGGTheoraKeyframeFrequency(x1 + 60, y, this);
	keyframe_frequency->create_objects();
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Keyframe force frequency:")));
	OGGTheoraKeyframeForceFrequency *keyframe_force_frequency = 
		new OGGTheoraKeyframeForceFrequency(x1 + 60, y, this);
	keyframe_force_frequency->create_objects();
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Sharpness:")));
	OGGTheoraSharpness *sharpness = 
		new OGGTheoraSharpness(x1 + 60, y, this);
	sharpness->create_objects();
	y += 30;

	add_subwindow(new BC_OKButton(this));
}


void OGGConfigVideo::close_event()
{
	set_done(0);
}

OGGTheoraBitrate::OGGTheoraBitrate(int x, int y, OGGConfigVideo *gui)
 : BC_TextBox(x, y, 100, 1, gui->asset->theora_bitrate)
{
	this->gui = gui;
}

int OGGTheoraBitrate::handle_event()
{
	// TODO: MIN / MAX check
	gui->asset->theora_bitrate = atol(get_text());
	return 1;
};


OGGTheoraFixedBitrate::OGGTheoraFixedBitrate(int x, int y, OGGConfigVideo *gui)
 : BC_Radial(x, y, gui->asset->theora_fix_bitrate, _("Fixed bitrate"))
{
	this->gui = gui;
}

int OGGTheoraFixedBitrate::handle_event()
{
	update(1);
	gui->asset->theora_fix_bitrate = 1;
	gui->fixed_quality->update(0);
	return 1;
};


OGGTheoraFixedQuality::OGGTheoraFixedQuality(int x, int y, OGGConfigVideo *gui)
 : BC_Radial(x, y, !gui->asset->theora_fix_bitrate, _("Fixed quality"))
{
	this->gui = gui;
}

int OGGTheoraFixedQuality::handle_event()
{
	update(1);
	gui->asset->theora_fix_bitrate = 0;
	gui->fixed_bitrate->update(0);
	return 1;
};


OGGTheoraKeyframeFrequency::OGGTheoraKeyframeFrequency(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
	(int64_t)gui->asset->theora_keyframe_frequency, 
	(int64_t)1,
	(int64_t)500,
	x, 
	y,
	40)
{
	this->gui = gui;
}

int OGGTheoraKeyframeFrequency::handle_event()
{
	gui->asset->theora_keyframe_frequency = atol(get_text());
	return 1;
}

OGGTheoraKeyframeForceFrequency::OGGTheoraKeyframeForceFrequency(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
	(int64_t)gui->asset->theora_keyframe_frequency, 
	(int64_t)1,
	(int64_t)500,
	x, 
	y,
	40)
{
	this->gui = gui;
}

int OGGTheoraKeyframeForceFrequency::handle_event()
{
	gui->asset->theora_keyframe_frequency = atol(get_text());
	return 1;
}


OGGTheoraSharpness::OGGTheoraSharpness(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
	(int64_t)gui->asset->theora_sharpness, 
	(int64_t)0,
	(int64_t)2,
	x, 
	y,
	40)
{
	this->gui = gui;
}

int OGGTheoraSharpness::handle_event()
{
	gui->asset->theora_sharpness = atol(get_text());
	return 1;
}

// Callbacks for TOC generation
int FileOGG::get_streamcount()
{
	int count = 0;

	for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
	{
		if(cur_stream->ti || cur_stream->vi)
			count++;
	}
	return count;
}

stream_params *FileOGG::get_track_data(int trx)
{
	ogg_int64_t gpos;
	int f_pkt = 1;
	int interrupt = 0;

	samplenum ix_ival;
	int samples;
	int f_bit;
	posnum count, next_count;
	off_t fpgoffs;
	off_t pkt_endpage;
	off_t pageoffs;

	if(!trx)
		cur_stream = streams;
	else
		cur_stream++;
	while(cur_stream->ti == 0 && cur_stream->vi == 0)
	{
		if(++cur_stream >= &streams[free_stream])
			return 0;
	}
	if(cur_stream->ti)
	{
		track_data.type = TOCSTREAM_VIDEO;
		track_data.rate_num = cur_stream->ti->fps_numerator;
		track_data.rate_denom = cur_stream->ti->fps_denominator;
		track_data.layers = 1;
	}
	else
	{
		track_data.type = TOCSTREAM_AUDIO;
		track_data.rate_num = cur_stream->vi->rate;
		track_data.rate_denom = 0;
		track_data.layers = cur_stream->vi->channels;
	}
	track_data.id = cur_stream->state.serialno;
	track_data.data1 = 0;
	track_data.data2 = 0;
	track_data.min_index = 0;
	track_data.min_offset = 0;
	track_data.index_correction = 0;

	read_buffer_at(filedata_begin);
	ogg_stream_reset(&cur_stream->state);
	gpos = -1;
	fpgoffs = 0;
	pkt_endpage = 0;
	count = next_count = 0;

	if(cur_stream->vi)
	{
		vorbis_synthesis_restart(&cur_stream->vs);
		ix_ival = OGG_INDEX_INTERVAL * cur_stream->vi->rate;
	}

	while(next_oggpage(&page) == 0)
	{
		if(ogg_page_serialno(&page) == cur_stream->state.serialno)
		{
			pageoffs = file_pagestart;

			if(track_data.min_offset == 0)
				track_data.min_offset = pageoffs;

			if(!(page.header[5] & 1))
			{
				fpgoffs = file_pagestart;
				f_bit = 1;
			}
			else
				f_bit = 0;

			ogg_stream_pagein(&cur_stream->state, &page);

			while(ogg_stream_packetout(&cur_stream->state, &pkt) == 1)
			{
				gpos = -1;
				if(f_pkt && pkt.granulepos > 0)
				{
					gpos = pkt.granulepos;
					f_pkt = 0;
				}
				if(cur_stream->ti)
				{
					if(theora_packet_iskeyframe(&pkt))
						interrupt = tocfile->append_item(count,
							MAX(fpgoffs, pkt_endpage));
					count++;
				}
				else if(cur_stream->vi)
				{
					if(vorbis_synthesis(&cur_stream->block, &pkt) ||
							vorbis_synthesis_blockin(&cur_stream->vs, &cur_stream->block))
					{
						errorbox("Failed to feed packet to vorbis decoder");
						return 0;
					}
					samples =  vorbis_synthesis_pcmout(&cur_stream->vs, 0);
					count += samples;
					vorbis_synthesis_read(&cur_stream->vs, samples);
					if(f_bit && count >= next_count)
					{
						interrupt = tocfile->append_item(count, pageoffs);
						next_count = count + ix_ival;
					}
				}
				if(gpos > 0)
				{
					if(cur_stream->ti)
					{
						int kfgshift = theora_granule_shift(cur_stream->ti);
						gpos = (gpos >> kfgshift) + (gpos & ~(-1 << kfgshift));
					}

					if(gpos > count)
					{
						track_data.index_correction = gpos - count;
						track_data.min_index = track_data.index_correction;
					}
				}
				pkt_endpage = pageoffs;

				// Return immediately if user canceled process
				if(interrupt)
					return 0;
			}
			if(ogg_page_eos(&page))
				break;
		}
	}
	track_data.max_offset = file_pagepos;
	// Index is valid for known stream types
	if(cur_stream->ti || cur_stream->vi)
		track_data.max_index = count + track_data.index_correction;

	return &track_data;
}
