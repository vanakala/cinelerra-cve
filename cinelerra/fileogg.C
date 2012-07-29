
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
#include "bcsignals.h"
#include "byteorder.h"
#include "clip.h"
#include "edit.h"
#include "file.h"
#include "fileogg.h"
#include "filetoc.h"
#include "guicast.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.inc"
#include "quicktime.h"
#include "vframe.h"
#include "videodevice.inc"
#include "cmodel_permutation.h"
#include "interlacemodes.h"
#include "mainerror.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Needed for packaging engine
#include "preferences.h"
#include "render.h"

#define READ_SIZE 66000


FileOGG::FileOGG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_OGG;
	asset->byte_order = 0;
	reset_parameters();
	final_write = 1;
	free_stream = 0;
	open_streams = 0;
	configured_streams = 0;
	tf = 0;
	temp_frame = 0;
	stream = 0;
	flush_lock = 0;
	tocfile = 0;
	memset(&track_data, 0, sizeof(track_data));
}

FileOGG::~FileOGG()
{
	if(tf)
	{
		if(tf->videosync) 
		{
			ogg_sync_clear(&tf->videosync->sync);
			delete tf->videosync;
			theora_info_clear(&tf->ti);
			theora_comment_clear(&tf->tc);
		}
		if(tf->audiosync) 
		{
			ogg_sync_clear(&tf->audiosync->sync);
			delete tf->audiosync;
			vorbis_info_clear(&tf->vi);
			vorbis_comment_clear(&tf->vc);
		}
		if(tf->vpage)
			free(tf->vpage);
		if(tf->apage)
			free(tf->apage);
		delete tf;
	}
	if(temp_frame) delete temp_frame;
	if(stream) close_file();
	if(flush_lock) delete flush_lock;
}

void FileOGG::get_parameters(BC_WindowBase *parent_window,
	Asset *asset,
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(audio_options)
	{
		OGGConfigAudio *window = new OGGConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options)
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

int FileOGG::open_file(int rd, int wr)
{
	int ret = 0;

	this->rd = rd;
	this->wr = wr;
	if(!tf)
	{
		tf = new theoraframes_info_t;
		memset(tf, 0, sizeof(*tf));
	}
	free_stream = 0;

	memset(streams, 0, sizeof(streams));

	if(wr)
	{
		// FIXIT: Temporarily disabled
		errorbox("FileOGG: encoding is not ready");
		return 1;

		if((stream = fopen(asset->path, "w+b")) == 0)
		{
			errormsg("Error while opening \"%s\" for writing. %m", asset->path);
			return 1;
		}

		tf->audio_bytesout = 0;
		tf->video_bytesout = 0;
		tf->videotime = 0;
		tf->audiotime = 0;

		tf->vpage_valid = 0;
		tf->apage_valid = 0;
		tf->apage_buffer_length = 0;
		tf->vpage_buffer_length = 0;
		tf->apage = NULL;
		tf->vpage = NULL;
		tf->v_pkg=0;
		tf->a_pkg=0;

		/* yayness.  Set up Ogg output stream */
		srand (time (NULL));

		if(asset->video_data)
		{
			ogg_stream_init (&tf->to, rand ());    /* oops, add one ot the above */

			theora_info_init (&tf->ti);

			tf->ti.frame_width = asset->width; 
			tf->ti.frame_height = asset->height;

			tf->ti.width = ((asset->width + 15) >>4)<<4; // round up to the nearest multiple of 16 
			tf->ti.height = ((asset->height + 15) >>4)<<4; // round up to the nearest multiple of 16
			if(tf->ti.width != tf->ti.frame_width || tf->ti.height != tf->ti.frame_height)
			{
				errormsg("WARNING: Encoding theora when width or height are not dividable by 16 is suboptimal");
			}

			tf->ti.offset_x = 0;
			tf->ti.offset_y = tf->ti.height - tf->ti.frame_height;
			tf->ti.fps_numerator = (unsigned int)(asset->frame_rate * 1000000);
			tf->ti.fps_denominator = 1000000;

			if(asset->aspect_ratio > 0)
			{
				// Cinelerra uses frame aspect ratio, theora uses pixel aspect ratio
				float pixel_aspect = asset->aspect_ratio / asset->width * asset->height;
				tf->ti.aspect_numerator = (unsigned int)(pixel_aspect * 1000000);
				tf->ti.aspect_denominator = 1000000;
			}
			else
			{
				tf->ti.aspect_numerator = 1000000;
				tf->ti.aspect_denominator = 1000000;
			}
			if(EQUIV(asset->frame_rate, 25) || EQUIV(asset->frame_rate, 50))
				tf->ti.colorspace = OC_CS_ITU_REC_470BG;
			else if((asset->frame_rate > 29 && asset->frame_rate < 31) || (asset->frame_rate > 59 && asset->frame_rate < 61) )
				tf->ti.colorspace = OC_CS_ITU_REC_470M;
			else
				tf->ti.colorspace = OC_CS_UNSPECIFIED;

			if(asset->theora_fix_bitrate)
			{
				tf->ti.target_bitrate = asset->theora_bitrate; 
				tf->ti.quality = 0;
			}
			else
			{
				tf->ti.target_bitrate = 0;
				tf->ti.quality = asset->theora_quality;     // video quality 0-63
			}
			tf->ti.dropframes_p = 0;
			tf->ti.quick_p = 1;
			tf->ti.keyframe_auto_p = 1;
			tf->ti.keyframe_frequency = asset->theora_keyframe_frequency;
			tf->ti.keyframe_frequency_force = asset->theora_keyframe_force_frequency;
			tf->ti.keyframe_data_target_bitrate = (unsigned int) (tf->ti.target_bitrate * 1.5) ;
			tf->ti.keyframe_auto_threshold = 80;
			tf->ti.keyframe_mindistance = 8;
			tf->ti.noise_sensitivity = 1;
			tf->ti.sharpness = 2;

			if(theora_encode_init (&tf->td, &tf->ti))
			{
				errormsg("(FileOGG:file_open) initialization of theora codec failed");
			}
		}
		/* init theora done */

		/* initialize Vorbis too, if we have audio. */
		if(asset->audio_data)
		{
			ogg_stream_init (&tf->vo, rand ());
			vorbis_info_init (&tf->vi);
			/* Encoding using a VBR quality mode.  */
			int ret;
			if(!asset->vorbis_vbr) 
			{
				ret = vorbis_encode_init(&tf->vi, 
							asset->channels, 
							asset->sample_rate, 
							asset->vorbis_max_bitrate, 
							asset->vorbis_bitrate,
							asset->vorbis_min_bitrate); 
			}
			else
			{
				// Set true VBR as demonstrated by http://svn.xiph.org/trunk/vorbis/doc/vorbisenc/examples.html
				ret = vorbis_encode_setup_managed(&tf->vi,
					asset->channels, 
					asset->sample_rate, 
					-1, 
					asset->vorbis_bitrate, 
					-1);
				ret |= vorbis_encode_ctl(&tf->vi, OV_ECTL_RATEMANAGE_AVG, NULL);
				ret |= vorbis_encode_setup_init(&tf->vi);
			}

			if(ret)
			{
				errormsg("The Vorbis encoder could not set up a mode according to"
							"the requested quality or bitrate.");

				fclose (stream);
				stream = 0;
				return 1;
			}

			vorbis_comment_init(&tf->vc); // comment is cleared lateron 
			vorbis_comment_add_tag(&tf->vc, (char*)"ENCODER", (char*)PACKAGE_STRING);
			/* set up the analysis state and auxiliary encoding storage */
			vorbis_analysis_init(&tf->vd, &tf->vi);
			vorbis_block_init(&tf->vd, &tf->vb);
		}
		/* audio init done */

		/* write the bitstream header packets with proper page interleave */

		/* first packet will get its own page automatically */
		if(asset->video_data)
		{
			theora_encode_header (&tf->td, &tf->op);
			ogg_stream_packetin (&tf->to, &tf->op);
			if(ogg_stream_pageout (&tf->to, &tf->og) != 1)
			{
				errorbox("Internal Ogg library error.");
				return 1;
			}
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);

			/* create the remaining theora headers */
			theora_comment_init (&tf->tc);
			theora_comment_add_tag (&tf->tc, (char*)"ENCODER", (char*)PACKAGE_STRING);
			theora_encode_comment (&tf->tc, &tf->op);
			ogg_stream_packetin (&tf->to, &tf->op);
			theora_comment_clear(&tf->tc);
			theora_encode_tables (&tf->td, &tf->op);
			ogg_stream_packetin (&tf->to, &tf->op);
		}
		if(asset->audio_data)
		{
			ogg_packet header;
			ogg_packet header_comm;
			ogg_packet header_code;

			vorbis_analysis_headerout (&tf->vd, &tf->vc, &header,
				&header_comm, &header_code);
			ogg_stream_packetin (&tf->vo, &header);    /* automatically placed in its own page */
			vorbis_comment_clear(&tf->vc);
			if(ogg_stream_pageout (&tf->vo, &tf->og) != 1)
			{
				errorbox("Internal Ogg library error.");
				return 1;
			}
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);

			/* remaining vorbis header packets */
			ogg_stream_packetin (&tf->vo, &header_comm);
			ogg_stream_packetin (&tf->vo, &header_code);
		}

		/* Flush the rest of our headers. This ensures
		 * the actual data in each stream will start
		 * on a new page, as per spec. */
		while(asset->video_data)
		{
			int result = ogg_stream_flush (&tf->to, &tf->og);
			if(result < 0)
			{
				/* can't get here */
				errorbox("Internal Ogg library error.");
				return 1;
			}
			if(result == 0)
				break;
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);
		}
		while(asset->audio_data)
		{
			int result = ogg_stream_flush (&tf->vo, &tf->og);
			if(result < 0)
			{
				/* can't get here */
				errormsg("Internal Ogg library error.");
				return 1;
			}
			if(result == 0)
				break;
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);
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
		file_length = file_stat.st_size;

// Check existence of toc
		tocfile = new FileTOC(this, file->preferences->index_directory, asset->path,
			file_length, file_stat.st_mtime);
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

	if(!ret)
	{
		// Anything is in OGG container
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

void FileOGG::close_file()
{
	if(wr)
	{
		if(final_write)
		{
			if(asset->audio_data)
				write_samples_vorbis(0, 0, 1); // set eos
			if(asset->video_data)
				write_frames_theora(0, 1, 1); // set eos
		}
		flush_ogg(1); // flush all

		if(asset->audio_data)
		{
			vorbis_block_clear (&tf->vb);
			vorbis_dsp_clear (&tf->vd);
			vorbis_info_clear (&tf->vi);
			ogg_stream_clear (&tf->vo);
		}
		if(asset->video_data)
		{
			theora_info_clear (&tf->ti);
			ogg_stream_clear (&tf->to);
			theora_clear (&tf->td);
		}

		if(stream) fclose(stream);
		stream = 0;
	} 
	else if(rd)
	{
		media_stream_t *cur_stream;
		for(cur_stream = streams; cur_stream < &streams[free_stream]; cur_stream++)
		{
			if(cur_stream->ti)
			{
				theora_info_clear(cur_stream->ti);
				theora_comment_clear(cur_stream->tc);
				theora_clear(&cur_stream->ts);
				delete cur_stream->ti;
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
				delete cur_stream->vc;
				cur_stream->vi = 0;
			}
			ogg_stream_clear(&cur_stream->state);
			cur_stream->headers = 0;
			cur_stream->dec_init = 0;
		}
		if(tocfile) delete tocfile;
		ogg_packet_clear(&pkt);
		ogg_sync_destroy(&sync_state);
		if(stream) fclose(stream);
		stream = 0;
	}
}

int FileOGG::colormodel_supported(int colormodel)
{
	if(colormodel == BC_YUV420P)
		return BC_YUV420P;
	else
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
	frame->set_pts((ptstime)next_frame_position / asset->frame_rate);
	frame->set_duration(1. / asset->frame_rate);
	frame->set_frame_number(next_frame_position);
	return 0;
}

void FileOGG::set_audio_position(samplenum x)
{
	next_sample_position = x + start_sample;
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

int FileOGG::read_samples(double *buffer, int len)
{
	float **vorbis_buffer;
	int filled;

	next_sample_position = file->current_sample;

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
	while(filled < len)
	{
		if(fill_pcm_samples(len))
			return 0;

		float *input = &cur_stream->pcm_samples[file->current_channel][filled];
		int chunk = MIN(cur_stream->pcm_size, len - filled);
		for (int i = 0; i < chunk; i++)
			buffer[i] = input[i];

		filled += chunk;
		next_sample_position += chunk;
	}
	return 0;
}

int FileOGG::write_audio_page()
{
	int ret;

	ret = fwrite(tf->apage, 1, tf->apage_len, stream);
	if(ret < tf->apage_len) 
	{
		errorbox("Error writing OGG audio page");
	}
	tf->apage_valid = 0;
	tf->a_pkg -= ogg_page_packets((ogg_page *)&tf->apage);
	return ret;
}

int FileOGG::write_video_page()
{
	int ret;

	ret = fwrite(tf->vpage, 1, tf->vpage_len, stream);
	if(ret < tf->vpage_len) 
	{
		errorbox("error writing OGG video page");
	}
	tf->vpage_valid = 0;
	tf->v_pkg -= ogg_page_packets((ogg_page *)&tf->vpage);
	return ret;
}

void FileOGG::flush_ogg(int e_o_s)
{
	int len;
	ogg_page og;

	flush_lock->lock();
	/* flush out the ogg pages  */
	while(1)
	{
	/* Get pages for both streams, if not already present, and if available.*/
		if(asset->video_data && !tf->vpage_valid)
		{
			// this way seeking is much better,
			// not sure if 23 packets  is a good value. it works though
			int v_next=0;
			if(tf->v_pkg > 22 && ogg_stream_flush(&tf->to, &og) > 0)
				v_next=1;
			else if(ogg_stream_pageout(&tf->to, &og) > 0)
				v_next=1;

			if(v_next)
			{
				len = og.header_len + og.body_len;
				if(tf->vpage_buffer_length < len)
				{
					tf->vpage = (unsigned char *)realloc(tf->vpage, len);
					tf->vpage_buffer_length = len;
				}
				tf->vpage_len = len;
				memcpy(tf->vpage, og.header, og.header_len);
				memcpy(tf->vpage+og.header_len , og.body, og.body_len);

				tf->vpage_valid = 1;
				tf->videotime = theora_granule_time(&tf->td, ogg_page_granulepos(&og));
			}
		}
		if(asset->audio_data && !tf->apage_valid)
		{
			// this way seeking is much better,
			// not sure if 23 packets  is a good value. it works though
			int a_next=0;
			if(tf->a_pkg > 22 && ogg_stream_flush(&tf->vo, &og) > 0)
				a_next=1;
			else if(ogg_stream_pageout(&tf->vo, &og) > 0)
				a_next=1;

			if(a_next)
			{
				len = og.header_len + og.body_len;
				if(tf->apage_buffer_length < len)
				{
					tf->apage = (unsigned char *)realloc(tf->apage, len);
					tf->apage_buffer_length = len;
				}
				tf->apage_len = len;
				memcpy(tf->apage, og.header, og.header_len);
				memcpy(tf->apage+og.header_len , og.body, og.body_len);

				tf->apage_valid = 1;
				tf->audiotime= vorbis_granule_time (&tf->vd,
					ogg_page_granulepos(&og));
			}
		}

		if(!asset->audio_data && tf->vpage_valid)
			write_video_page();
		else if(!asset->video_data && tf->apage_valid)
			write_audio_page();

		/* We're using both. We can output only:
		 *  a) If we have valid pages for both
		 *  b) At EOS, for the remaining stream.
		 */
		else if(tf->vpage_valid && tf->apage_valid)
		{
			/* Make sure they're in the right order. */
			if(tf->videotime <= tf->audiotime)
				write_video_page();
			else
				write_audio_page();
		}
		else if(e_o_s && tf->vpage_valid)
			write_video_page();
		else if(e_o_s && tf->apage_valid)
			write_audio_page();
		else
			break; /* Nothing more writable at the moment */
	}
	flush_lock->unlock();
}

void FileOGG::write_samples_vorbis(double **buffer, int len, int e_o_s)
{
	int i,j, count = 0;
	float **vorbis_buffer;
	static int samples = 0;

	samples += len;
	if(e_o_s)
		vorbis_analysis_wrote(&tf->vd, 0);
	else
	{
		vorbis_buffer = vorbis_analysis_buffer (&tf->vd, len);
		/* double to float conversion */
		for(i = 0; i<asset->channels; i++)
		{
			for(j = 0; j < len; j++)
				vorbis_buffer[i][j] = buffer[i][j];
		}
		vorbis_analysis_wrote(&tf->vd, len);
	}
	while(vorbis_analysis_blockout(&tf->vd, &tf->vb) == 1)
	{
		/* analysis, assume we want to use bitrate management */
		vorbis_analysis(&tf->vb, NULL);
		vorbis_bitrate_addblock(&tf->vb);

		/* weld packets into the bitstream */
		while(vorbis_bitrate_flushpacket(&tf->vd, &tf->op))
		{
			flush_lock->lock();
			ogg_stream_packetin(&tf->vo, &tf->op);
			tf->a_pkg++;
			flush_lock->unlock();
		}
	}
	flush_ogg(0);
}

int FileOGG::write_samples(double **buffer, int len)
{
	if(len > 0)
		write_samples_vorbis(buffer, len, 0);
	return 0;
}

void FileOGG::write_frames_theora(VFrame ***frames, int len, int e_o_s)
{
	// due to clumsy theora's design we need to delay writing out by one frame
	// always stay one frame behind, so we can correctly encode e_o_s

	int i, j, result = 0;

	if(!stream) return;

	for(j = 0; j < len && !result; j++)
	{
		if(temp_frame) // encode previous frame if available
		{
			yuv_buffer yuv;
			yuv.y_width = tf->ti.width;
			yuv.y_height = tf->ti.height;
			yuv.y_stride = temp_frame->get_bytes_per_line();

			yuv.uv_width = tf->ti.width / 2;
			yuv.uv_height = tf->ti.height / 2;
			yuv.uv_stride = temp_frame->get_bytes_per_line() /2;

			yuv.y = temp_frame->get_y();
			yuv.u = temp_frame->get_u();
			yuv.v = temp_frame->get_v();
			int ret = theora_encode_YUVin (&tf->td, &yuv);
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
			}

			while(theora_encode_packetout (&tf->td, e_o_s, &tf->op)) {
				flush_lock->lock();
				ogg_stream_packetin (&tf->to, &tf->op);
				tf->v_pkg++;
				flush_lock->unlock();
			}
			flush_ogg(0);  // eos flush is done later at close_file
		}
// If we have e_o_s, don't encode any new frames
		if(e_o_s)
			break;

		if(!temp_frame)
		{
			temp_frame = new VFrame (0, 
						tf->ti.width, 
						tf->ti.height,
						BC_YUV420P);
		} 
		VFrame *frame = frames[0][j];
		int in_color_model = frame->get_color_model();
		if(in_color_model == BC_YUV422P &&
			temp_frame->get_w() == frame->get_w() &&
			temp_frame->get_h() == frame->get_h() &&
			temp_frame->get_bytes_per_line() == frame->get_bytes_per_line())
		{
			temp_frame->copy_from(frame);
		}
		else
		{
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
				frame->get_w(),  // temp_frame can be larger than frame if width not dividable by 16
				frame->get_h(),
				frame->get_color_model(),
				BC_YUV420P,
				0,
				frame->get_w(),
				temp_frame->get_w());
		}
	}
}

int FileOGG::write_frames(VFrame ***frames, int len)
{
	write_frames_theora(frames, len, 0);
	return 0;
}


OGGConfigAudio::OGGConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	350,
	250)
{
	this->parent_window = parent_window;
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
	this->parent_window = parent_window;
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


PackagingEngineOGG::PackagingEngineOGG()
{
	packages = 0;
	default_asset = 0;
}

PackagingEngineOGG::~PackagingEngineOGG()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
	if(default_asset)
		delete default_asset;
}

int PackagingEngineOGG::create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		ptstime total_start,
		ptstime total_end)
{
	this->total_start = total_start;
	this->total_end = total_end;
	this->edl = edl;

	this->preferences = preferences;

// We make A COPY of the asset, because we set audio_data = 0 on local asset which is the same copy as default_asset... 
// Should be taken care of somewhere else actually
	this->default_asset = new Asset(*default_asset);

	audio_start_pts = total_start;
	video_start_pts = total_start;
	audio_pts = audio_start_pts;
	video_pts = video_start_pts;
	audio_end_pts = total_end;
	video_end_pts = total_end;
	current_package = 0;

	double total_len = total_end - total_start;

	total_packages = 0;
	if(default_asset->audio_data)
		total_packages++;
	if(default_asset->video_data)
		total_packages += preferences->renderfarm_job_count;

	packages = new RenderPackage*[total_packages];

	int local_current_package = 0;
	if(default_asset->audio_data)
	{
		packages[local_current_package] = new RenderPackage;
		sprintf(packages[current_package]->path, "%s.audio", default_asset->path);
		local_current_package++;
	}

	if(default_asset->video_data)
	{
		video_package_len = (total_len) / preferences->renderfarm_job_count;
		int current_number;    // The number being injected into the filename.
		int number_start;      // Character in the filename path at which the number begins
		int total_digits;      // Total number of digits including padding the user specified.

		Render::get_starting_number(default_asset->path, 
			current_number,
			number_start, 
			total_digits,
			3);

		for(int i = 0; i < preferences->renderfarm_job_count; i++)
		{
			RenderPackage *package = packages[local_current_package] = new RenderPackage;
			Render::create_filename(package->path, 
				default_asset->path, 
				current_number,
				total_digits,
				number_start);
			current_number++;
			local_current_package++;
		}
	}
}

RenderPackage* PackagingEngineOGG::get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate)
{
	if(current_package == total_packages)
		return 0;

	RenderPackage *result = 0;
	if(current_package == 0 && default_asset->audio_data)
	{
		result = packages[0];
		result->audio_start_pts = audio_start_pts;
		result->video_start_pts = video_start_pts;
		result->audio_end_pts = audio_end_pts;
		result->video_end_pts = video_end_pts;
		result->audio_do = 1;
		result->video_do = 0;
	}
	else if(default_asset->video_data)
	{
		// Do not do any scaling according to node speed, so we know we can get evenly distributed 'forced' keyframes
		result = packages[current_package];
		result->audio_do = 0;
		result->video_do = 1;

		result->audio_start_pts = audio_pts;
		result->video_start_pts = video_pts;
		result->audio_end_pts = result->audio_start_pts + video_package_len;
		result->video_end_pts = result->video_start_pts + video_package_len;

// Last package... take it all!
		if(current_package == total_packages - 1)
		{
			result->audio_end_pts = audio_end_pts;
			result->video_end_pts = video_end_pts;
		}

		audio_pts = result->audio_end_pts;
		video_pts = result->video_end_pts;
	}

	current_package++;
	return result;
}

void PackagingEngineOGG::get_package_paths(ArrayList<char*> *path_list)
{
	for(int i = 0; i < total_packages; i++)
		path_list->append(strdup(packages[i]->path));

// We will mux to the the final file at the end!
	path_list->append(strdup(default_asset->path));
	path_list->set_free();
}

ptstime PackagingEngineOGG::get_progress_max()
{
	return (total_end - total_start) * 2 +
		preferences->render_preroll * total_packages +
		(preferences->render_preroll >= total_start ?
		(total_start - preferences->render_preroll) * 2 : 0);
}

int PackagingEngineOGG::packages_are_done()
{
// Mux audio and video into one file
	Asset *video_asset, *audio_asset;
	File *audio_file_gen, *video_file_gen;
	FileOGG *video_file, *audio_file;
	ogg_stream_state audio_in_stream, video_in_stream;

	int local_current_package = 0;
	if(default_asset->audio_data)
	{
		audio_asset = new Asset(packages[local_current_package]->path);
		local_current_package++;
		audio_file_gen = new File();
		audio_file_gen->open_file(preferences, 
			audio_asset, 
			1, //rd 
			0, //wr
			0, //base sample rate
			0); // base_frame rate
		audio_file = (FileOGG*) audio_file_gen->file;
		ogg_stream_init(&audio_in_stream, audio_file->tf->vo.serialno);
/* FIXIT
		audio_file->seek_to_databegin(audio_file->tf->audiosync, audio_file->tf->vo.serialno);
	*/
	}

	if(default_asset->video_data)
	{
		video_asset = new Asset(packages[local_current_package]->path);
		local_current_package++;
		video_file_gen = new File();
		video_file_gen->open_file(preferences, 
			video_asset, 
			1, //rd 
			0, //wr
			0, //base sample rate
			0); // base_frame rate
		video_file = (FileOGG*) video_file_gen->file;
		ogg_stream_init(&video_in_stream, video_file->tf->to.serialno);
/* FIXIT
		video_file->seek_to_databegin(video_file->tf->videosync, video_file->tf->to.serialno);
	*/
	}
// Output file
	File *output_file_gen = new File();
	output_file_gen->open_file(preferences,
		default_asset,
		0,
		1,
		default_asset->sample_rate, 
		default_asset->frame_rate);
	FileOGG *output_file = (FileOGG*)output_file_gen->file;

	ogg_page og;    /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet op;  /* one raw packet of data for decode */

	int audio_ready = default_asset->audio_data;
	int video_ready = default_asset->video_data;
	ogg_int64_t video_packetno = 1;
	ogg_int64_t audio_packetno = 1;
	ogg_int64_t frame_offset = 0;
	ogg_int64_t current_frame = 0;
/* FIXIT
	while((default_asset->audio_data && audio_ready) || (default_asset->video_data && video_ready))
	{
		if(video_ready)
		{
			while(ogg_stream_packetpeek(&video_in_stream, NULL) != 1) // get as many pages as needed for one package
			{
				if(!video_file->get_next_page(video_file->tf->videosync, video_file->tf->to.serialno, &video_file->tf->videopage))
				{
					// We are at the end of our file, see if it is more and open more if there is
					if(local_current_package < total_packages)
					{
						frame_offset = current_frame +1;
						ogg_stream_clear(&video_in_stream);
						video_file_gen->close_file();
						delete video_file_gen;
						Garbage::delete_object(video_asset);
						video_asset = new Asset(packages[local_current_package]->path);
						local_current_package++;

						video_file_gen = new File();
						video_file_gen->open_file(preferences, 
							video_asset, 
							1, //rd 
							0, //wr
							0, //base sample rate
							0); // base_frame rate
						video_file = (FileOGG*)video_file_gen->file;
						ogg_stream_init(&video_in_stream, video_file->tf->to.serialno);
						video_file->seek_to_databegin(video_file->tf->videosync, video_file->tf->to.serialno);
					} else
						video_ready = 0;
					break;
				}
				ogg_stream_pagein(&video_in_stream, &video_file->tf->videopage);
			}
			while(ogg_stream_packetpeek(&video_in_stream, NULL) == 1) // get all packets out of the page
			{
				ogg_stream_packetout(&video_in_stream, &op);
				if(local_current_package != total_packages) // keep it from closing the stream
					op.e_o_s = 0;
				if(video_packetno != 1)                     // if this is not the first video package do not start with b_o_s
					op.b_o_s = 0;
				else
					op.b_o_s = 1;
				op.packetno = video_packetno;
				video_packetno ++;
				ogg_int64_t granulepos = op.granulepos;
				if(granulepos != -1)
				{
				// Fix granulepos!
					ogg_int64_t rel_iframe = granulepos >> video_file->theora_keyframe_granule_shift;
					ogg_int64_t rel_pframe = granulepos - (rel_iframe << video_file->theora_keyframe_granule_shift);
					ogg_int64_t rel_current_frame = rel_iframe + rel_pframe;
					current_frame = frame_offset + rel_current_frame;
					ogg_int64_t abs_iframe = current_frame - rel_pframe;

					op.granulepos = (abs_iframe << video_file->theora_keyframe_granule_shift) + rel_pframe;
				}
				ogg_stream_packetin(&output_file->tf->to, &op);
				output_file->tf->v_pkg++; 
			}
		}
		if(audio_ready)
		{
			while(ogg_stream_packetpeek(&audio_in_stream, NULL) != 1) // get as many pages as needed for one package
			{
				if(!audio_file->get_next_page(audio_file->tf->audiosync, audio_file->tf->vo.serialno, &audio_file->tf->audiopage))
				{
					audio_ready = 0;
					break;
				}
				ogg_stream_pagein(&audio_in_stream, &audio_file->tf->audiopage);
			}
			while(ogg_stream_packetpeek(&audio_in_stream, NULL) == 1) // get all packets out of the page
			{
				ogg_stream_packetout(&audio_in_stream, &op);
				ogg_stream_packetin (&output_file->tf->vo, &op);
				audio_packetno++;
				output_file->tf->a_pkg++; 
			}
		}
		output_file->flush_ogg(0);
	}
	*/
// Just prevent thet write_samples and write_frames are called
	output_file->final_write = 0;

	if(default_asset->audio_data)
	{
		ogg_stream_clear(&audio_in_stream);
		audio_file_gen->close_file();
		delete audio_file_gen;
		Garbage::delete_object(audio_asset);
	}
	if(default_asset->video_data)
	{
		ogg_stream_clear(&video_in_stream);
		video_file_gen->close_file();
		delete video_file_gen;
		Garbage::delete_object(video_asset);
	}
	output_file_gen->close_file();
	delete output_file_gen;

// Now delete the temp files
	for(int i = 0; i < total_packages; i++)
		unlink(packages[i]->path);
	return 0;
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
