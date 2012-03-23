
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

/* This code was aspired by ffmpeg2theora */
/* Special thanks for help on this code goes out to j@v2v.cc */


FileOGG::FileOGG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_OGG;
	asset->byte_order = 0;
	reset_parameters();
	final_write = 1;
}

FileOGG::~FileOGG()
{
	if (tf) 
	{
		if (tf->videosync) 
		{
			ogg_sync_clear(&tf->videosync->sync);
			delete tf->videosync;
			theora_info_clear(&tf->ti);
			theora_comment_clear(&tf->tc);
		}
		if (tf->audiosync) 
		{
			ogg_sync_clear(&tf->audiosync->sync);
			delete tf->audiosync;
			vorbis_info_clear(&tf->vi);
			vorbis_comment_clear(&tf->vc);
		}
		if (tf->vpage)
			free(tf->vpage);
		if (tf->apage)
			free(tf->apage);
		delete tf;
	}
	if (temp_frame) delete temp_frame;
	if (stream) close_file();
	if(pcm_history)
	{
		for(int i = 0; i < asset->channels; i++)
			delete [] pcm_history[i];
		delete [] pcm_history;
	}

	if (flush_lock) delete flush_lock;
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

void FileOGG::reset_parameters_derived()
{
	tf = 0;
	temp_frame = 0;
	stream = 0;
	flush_lock = 0;
	pcm_history = 0;
}

static int read_buffer(FILE *in, sync_window_t *sw, int buflen)
{
	char *buffer = ogg_sync_buffer(&sw->sync, buflen);

	sw->wlen = fread(buffer, 1, buflen, in);
	ogg_sync_wrote(&sw->sync, sw->wlen);
	sw->file_bufpos += sw->wlen;
	return (sw->wlen);
}

static int read_buffer_at(FILE *in, sync_window_t *sw, int buflen, off_t filepos)
{
	fseeko(in, filepos, SEEK_SET);
	sw->file_bufpos = filepos;
	sw->file_pagepos = filepos; // this one is not valid until sync_pageseek!
	ogg_sync_reset(&sw->sync);
	return read_buffer(in, sw, buflen);
}

static int take_page_out_autoadvance(FILE *in, sync_window_t *sw, ogg_page *og)
{
	while (1)
	{
		int ret = ogg_sync_pageout(&sw->sync, og);
		if (ret > 0)
		{
// advance 'virtual' position
			sw->file_pagepos += og->header_len + og->body_len; 
			return ret;
		}
		else if (ret < 0)
		{
			errormsg("FileOGG: Taking page out on nonsynced stream!\n");
			return ret;
		} else
		{
			// need more data for page
			if (read_buffer(in, sw, READ_SIZE) == 0) 
			{
				return 0;  // No more data
			}
		}
	}
	return 1;
}

// we never need to autoadvance when syncing, since our read chunks are larger than 
// maximum page size
static int sync_and_take_page_out(sync_window_t *sw, ogg_page *page)
{
	page->header_len = 0;
	page->body_len = 0;
	page->header = 0;
	page->body = 0;
	int ret = ogg_sync_pageseek(&sw->sync, page);
	if (ret < 0)
	{
		sw->file_pagepos -= ret;
	}
	else if (ret > 0)
	{
		sw->file_pagepos += ret;
	}
	return ret;
}


int FileOGG::open_file(int rd, int wr)
{
	this->rd = rd;
	this->wr = wr;
	if (!tf)
	{
		tf = new theoraframes_info_t;
		memset(tf, 0, sizeof(*tf));
	}

	if(wr)
	{
		if((stream = fopen(asset->path, "w+b")) == 0)
		{
			errormsg("Error while opening \"%s\" for writing. %m\n", asset->path);
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
			if (tf->ti.width != tf->ti.frame_width || tf->ti.height != tf->ti.frame_height)
			{
				errormsg("WARNING: Encoding theora when width or height are not dividable by 16 is suboptimal\n");
			}

			tf->ti.offset_x = 0;
			tf->ti.offset_y = tf->ti.height - tf->ti.frame_height;
			tf->ti.fps_numerator = (unsigned int)(asset->frame_rate * 1000000);
			tf->ti.fps_denominator = 1000000;

			if (asset->aspect_ratio > 0)
			{
				// Cinelerra uses frame aspect ratio, theora uses pixel aspect ratio
				float pixel_aspect = asset->aspect_ratio / asset->width * asset->height;
				tf->ti.aspect_numerator = (unsigned int)(pixel_aspect * 1000000);
				tf->ti.aspect_denominator = 1000000;
			} else
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

			if (asset->theora_fix_bitrate)
			{
				tf->ti.target_bitrate = asset->theora_bitrate; 
				tf->ti.quality = 0;
			} else
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

			if (theora_encode_init (&tf->td, &tf->ti))
			{
				errormsg("(FileOGG:file_open) initialization of theora codec failed\n");
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
			} else
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

			if (ret)
			{
				errormsg("The Vorbis encoder could not set up a mode according to\n"
							"the requested quality or bitrate.\n\n");

				fclose (stream);
				stream = 0;
				return 1;
			}

			vorbis_comment_init (&tf->vc); // comment is cleared lateron 
			vorbis_comment_add_tag (&tf->vc, (char*)"ENCODER", (char*)PACKAGE_STRING);
			/* set up the analysis state and auxiliary encoding storage */
			vorbis_analysis_init (&tf->vd, &tf->vi);
			vorbis_block_init (&tf->vd, &tf->vb);

		}
		/* audio init done */

		/* write the bitstream header packets with proper page interleave */

		/* first packet will get its own page automatically */
		if(asset->video_data)
		{
			theora_encode_header (&tf->td, &tf->op);
			ogg_stream_packetin (&tf->to, &tf->op);
			if (ogg_stream_pageout (&tf->to, &tf->og) != 1)
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
			if (ogg_stream_pageout (&tf->vo, &tf->og) != 1)
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
		while (1 && asset->video_data)
		{
			int result = ogg_stream_flush (&tf->to, &tf->og);
			if (result < 0)
			{
				/* can't get here */
				errorbox("Internal Ogg library error.");
				return 1;
			}
			if (result == 0)
				break;
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);
		}
		while (1 && asset->audio_data)
		{
			int result = ogg_stream_flush (&tf->vo, &tf->og);
			if (result < 0)
			{
				/* can't get here */
				errormsg("Internal Ogg library error.");
				return 1;
			}
			if (result == 0)
				break;
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);
		}
		flush_lock = new Mutex("OGGFile::Flush lock");
	} else
	if (rd)
	{

		if((stream = fopen(asset->path, "rb")) == 0)
		{
			errormsg("Error while opening %s for reading. %m", asset->path);
			return 1;
		}

		/* get file length */
		struct stat file_stat;
		stat(asset->path, &file_stat);
		file_length = file_stat.st_size;

		/* start up Ogg stream synchronization layer */
		/* oy is used just here to parse header, we use separate syncs for video and audio*/
		sync_window_t oy;
		ogg_sync_init(&oy.sync);
		// make sure we init the position structures to zero
		read_buffer_at(stream, &oy, READ_SIZE, 0);

		/* init supporting Vorbis structures needed in header parsing */
		vorbis_info_init(&tf->vi);
		vorbis_comment_init(&tf->vc);

		/* init supporting Theora structures needed in header parsing */
		theora_comment_init(&tf->tc);
		theora_info_init(&tf->ti);

		/* Ogg file open; parse the headers */
		/* Only interested in Vorbis/Theora streams */
		int stateflag = 0;
		int theora_p = 0;
		int vorbis_p = 0;
		while(!stateflag)
		{
			while(take_page_out_autoadvance(stream, &oy, &tf->og) > 0)
			{
				ogg_stream_state test;

				/* is this a mandated initial header? If not, stop parsing */
				if(!ogg_page_bos(&tf->og))
				{
					/* don't leak the page; get it into the appropriate stream */
					if(theora_p)ogg_stream_pagein(&tf->to, &tf->og);
					if(vorbis_p)ogg_stream_pagein(&tf->vo, &tf->og);

					stateflag = 1;
					break;
				}

				ogg_stream_init(&test, ogg_page_serialno(&tf->og));
				ogg_stream_pagein(&test, &tf->og);
				ogg_stream_packetout(&test, &tf->op);

				/* identify the codec: try theora */
				if(!theora_p && theora_decode_header(&tf->ti, &tf->tc, &tf->op)>=0)
				{
					/* it is theora */
					memcpy(&tf->to, &test, sizeof(test));
					theora_p = 1;
	// find out granule shift - from liboggz's oggz_auto.c
					unsigned char * header = tf->op.packet;
					theora_keyframe_granule_shift = (char) ((header[40] & 0x03) << 3);
					theora_keyframe_granule_shift |= (header[41] & 0xe0) >> 5;

				} else if(!vorbis_p && vorbis_synthesis_headerin(&tf->vi, &tf->vc, &tf->op)>=0)
				{
					/* it is vorbis */
					memcpy(&tf->vo, &test, sizeof(test));
					vorbis_p = 1;
				} else 
				{
					/* whatever it is, we don't care about it */
					ogg_stream_clear(&test);
				}
			}
		/* fall through to non-bos page parsing */
		}

		/* we're expecting more header packets. */
		while((theora_p && theora_p < 3) || (vorbis_p && vorbis_p < 3))
		{
			int ret;

			/* look for further theora headers */
			while(theora_p && (theora_p < 3) && (ret = ogg_stream_packetout(&tf->to, &tf->op)))
			{
				if(ret < 0)
				{
					errormsg("Error parsing Theora stream headers; corrupt stream?\n");
					return 1;
				}
				if(theora_decode_header(&tf->ti, &tf->tc, &tf->op))
				{
					errormsg("Error parsing Theora stream headers; corrupt stream?\n");
					return 1;
				}
				theora_p++;
				if(theora_p == 3) 
					break;
			}

			/* look for more vorbis header packets */
			while(vorbis_p && (vorbis_p < 3) && (ret = ogg_stream_packetout(&tf->vo, &tf->op)))
			{
				if(ret<0)
				{
					errormsg("Error parsing Vorbis stream headers; corrupt stream?\n");
					return 1;
				}
				if (vorbis_synthesis_headerin(&tf->vi, &tf->vc, &tf->op))
				{
					errormsg("Error parsing Vorbis stream headers; corrupt stream?\n");
					return 1;
				}
				vorbis_p++;
				if (vorbis_p == 3)
					break;
			}

			if ((!vorbis_p || vorbis_p == 3) && (!theora_p || theora_p == 3)) 
				break;
			/* The header pages/packets will arrive before anything else we
			    care about, or the stream is not obeying spec */

			if(take_page_out_autoadvance(stream, &oy, &tf->og) > 0)
			{
				if(theora_p) ogg_stream_pagein(&tf->to, &tf->og);
				if(vorbis_p) ogg_stream_pagein(&tf->vo, &tf->og);

			} else
			{
				errormsg("End of file while searching for codec headers.\n");
				return 1;
			}
		}
		// Remember where the real data begins for later seeking purposes
		filedata_begin = oy.file_pagepos; 

		/* and now we have it all.  initialize decoders */
		if(theora_p)
		{
			int ret;

// WORKAROUND for bug in alpha4 version of theora:
			tf->td.internal_encode = 0;

			ret = theora_decode_init(&tf->td, &tf->ti);
			double fps = (double)tf->ti.fps_numerator/tf->ti.fps_denominator;

			theora_cmodel = BC_YUV420P;

			if(tf->ti.width!=tf->ti.frame_width || tf->ti.height!=tf->ti.frame_height)
			{
				errormsg("Frame content is %dx%d with offset (%d,%d), We do not support this yet. You will get black border.\n",
							tf->ti.frame_width, tf->ti.frame_height, tf->ti.offset_x, tf->ti.offset_y);
			}
			tf->videosync = new sync_window_t;
			ogg_sync_init(&tf->videosync->sync);
			tf->videosync->wlen = 0;

			ret = ogg_get_first_page(tf->videosync, tf->to.serialno, &tf->videopage);
			ogg_packet op;

			// we have headers already decoded, so just skip them
			ogg_stream_reset(&tf->to);
			ogg_stream_pagein(&tf->to, &tf->videopage);
			while (1)
			{
				while (ogg_stream_packetpeek(&tf->to, NULL) != 1)
				{
					if (!ogg_get_next_page(tf->videosync, tf->to.serialno, &tf->videopage))
					{
						errormsg("Cannot find next page while looking for first non-header packet\n");
						return 1;
					}
					ogg_stream_pagein(&tf->to, &tf->videopage);
				}
				ogg_stream_packetout(&tf->to, &op);
				if (!theora_packet_isheader(&op))
					break;
			}
			// now get to the page of the finish of the first packet
			while (ogg_page_packets(&tf->videopage) == 0) 
			{
				if (ogg_page_granulepos(&tf->videopage) != -1)
				{
					errorbox("Broken ogg file - broken page: ogg_page_packets == 0 and granulepos != -1\n");
					return 1;
				}
				ogg_get_next_page(tf->videosync, tf->to.serialno, &tf->videopage);
			}
			// FIXME - LOW PRIORITY - start counting at first decodable keyframe!
			// but then we have to fix a/v sync somehow
			start_frame = (int64_t) (theora_granule_frame (&tf->td, ogg_page_granulepos(&tf->videopage)) - ogg_page_packets(&tf->videopage)) + 1;

			ret = ogg_get_last_page(tf->videosync, tf->to.serialno, &tf->videopage);
			last_frame = (int64_t) (theora_granule_frame (&tf->td, ogg_page_granulepos(&tf->videopage)));
			asset->video_length = last_frame - start_frame + 1;

			asset->layers = 1;
			// FIXME - LOW PRIORITY Cinelerra does not honor the frame_width and frame_height
			asset->width = tf->ti.width;
			asset->height = tf->ti.height;
// Don't want a user configured frame rate to get destroyed
			if(!asset->frame_rate)
				asset->frame_rate = fps;
// All theora material is noninterlaced by definition
			if(!asset->interlace_mode)
				asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

			set_video_position(0); // make sure seeking is done to the first sample
			ogg_frame_position = -10;
			asset->video_data = 1;
			strncpy(asset->vcodec, "theo", 4);

		} else
		{
			/* tear down the partial theora setup */
			theora_info_clear(&tf->ti);
			theora_comment_clear(&tf->tc);
		}

		if(vorbis_p)
		{
			vorbis_synthesis_init(&tf->vd, &tf->vi);
			vorbis_block_init(&tf->vd, &tf->vb);

			/* init audio_sync structure */
			tf->audiosync = new sync_window_t;
			ogg_sync_init(&tf->audiosync->sync);
			tf->audiosync->wlen = 0;

			int ret2 = ogg_get_first_page(tf->audiosync, tf->vo.serialno, &tf->audiopage);
			int result;
			ogg_packet op;
			ogg_stream_reset(&tf->vo);
			// FIXME - LOW PRIORITY should be getting pages until one has granulepos, probably never happens in pracitce
			ogg_stream_pagein(&tf->vo, &tf->audiopage);
			ogg_int64_t accumulated = 0;
			long        lastblock = -1;
			while((result = ogg_stream_packetout(&tf->vo, &op)))
			{
				if(result > 0)
				{ // ignore holes 
					long thisblock =  vorbis_packet_blocksize(&tf->vi, &op);
					if(lastblock != -1)
						accumulated += (lastblock + thisblock) >> 2;
					lastblock = thisblock;
				}
			}
			start_sample = ogg_page_granulepos(&tf->audiopage) - accumulated;

			int ret = ogg_get_last_page(tf->audiosync, tf->vo.serialno, &tf->audiopage);
			last_sample = ogg_page_granulepos(&tf->audiopage);
			asset->audio_length = last_sample - start_sample;

			asset->channels = tf->vi.channels;
			if(!asset->sample_rate)
				asset->sample_rate = tf->vi.rate;
			asset->audio_data = 1;

			ogg_sample_position = -10;
			set_audio_position(0); // make sure seeking is done to the first sample
			strncpy(asset->acodec, "vorb", 4);

		} else 
		{
			/* tear down the partial vorbis setup */
			vorbis_info_clear(&tf->vi);
			vorbis_comment_clear(&tf->vc);
		}
		ogg_sync_clear(&oy.sync);
	}
	return 0;
}

int FileOGG::ogg_get_prev_page(sync_window_t *sw, long serialno, ogg_page *og)
{
	char *buffer;
	ogg_page page;
	off_t filepos = sw->file_pagepos_found - READ_SIZE;
	int first_page_offset = 0;
	int done = 0;	
	int read_len = READ_SIZE;

	while (!done)
	{
		if (filepos < 0)
		{
			read_len = read_len + filepos;
			filepos = 0;
		}
		if (read_len <= 0) 
			return 0;
		int have_read = read_buffer_at(stream, sw, read_len, filepos);

		if (!have_read) 
			return 0;

// read all pages in the buffer
		int page_offset = 0;
		int page_length = 0;
		int first_page = 1;
		while (first_page || page_length) 
		{
			// if negative, skip bytes
			while ((page_length = sync_and_take_page_out(sw, &page)) < 0)
			{
				page_offset -= page_length;
			}
			if (first_page)
				first_page_offset = page_offset;
			first_page = 0;
			if (page_length && ogg_page_serialno(&page) == serialno)
			{
				// we will copy every page until last page in this buffer
				done = 1;
				
				sw->file_pagepos_found = sw->file_pagepos - page.header_len - page.body_len;
				memcpy(og, &page, sizeof(page));
			}
		}
		off_t old_filepos = filepos;
		filepos = filepos + first_page_offset - READ_SIZE;
	}

	if (done) 
		return 1;
	else 
		return 0;
}

int FileOGG::ogg_get_last_page(sync_window_t *sw, long serialno, ogg_page *og)
{
	char *buffer;
	ogg_page page;
	off_t filepos = file_length - READ_SIZE;
	if (filepos < 0) 
		filepos = 0;

	int first_page_offset = 0;
	int done = 0;
	while (!done && filepos >= 0)
	{
		int readlen;
		readlen = read_buffer_at(stream, sw, READ_SIZE, filepos);

// read all pages in the buffer
		int page_offset = 0;
		int page_length = 0;
		int first_page = 1;
		while (first_page || page_length) 
		{
			// if negative, skip bytes
			while ((page_length = sync_and_take_page_out(sw, &page)) < 0)
					page_offset -= page_length;
			if (first_page)
				first_page_offset = page_offset;
			first_page = 0;
			if (page_length && ogg_page_serialno(&page) == serialno)
			{
				// we will copy every page until last page in this buffer
				done = 1;
				sw->file_pagepos_found = sw->file_pagepos - page.header_len - page.body_len;
				memcpy(og, &page, sizeof(page));
			}
		}
		filepos = filepos + first_page_offset - READ_SIZE;
	}

	if (done) 
		return 1;
	else 
		return 0;
}

int FileOGG::ogg_get_first_page(sync_window_t *sw, long serialno, ogg_page *og)
{
	read_buffer_at(stream, sw, READ_SIZE, 0);
// we don't even need to sync since we _know_ it is right
	return (ogg_get_next_page(sw, serialno, og));
}

int FileOGG::ogg_seek_to_databegin(sync_window_t *sw, long serialno)
{
	read_buffer_at(stream, sw, READ_SIZE, filedata_begin);
// we don't even need to sync since we _know_ it is right
	return (0);
}

int FileOGG::ogg_get_next_page(sync_window_t *sw, long serialno, ogg_page *og)
{
	while (take_page_out_autoadvance(stream, sw, og) > 0)
	{ 
		if (ogg_page_serialno(og) == serialno)
		{
			sw->file_pagepos_found = sw->file_pagepos - og->header_len - og->body_len;
			return 1;
		}
	}
	return 0;
}

int FileOGG::ogg_sync_and_get_next_page(sync_window_t *sw, long serialno, ogg_page *og)
{
// TODO: Put better error reporting inhere
	int ret;
	while ((ret = sync_and_take_page_out(sw, og)) < 0)
	{
		// do nothing;
	}
	if (ret == 0) 
		return 0;
	if (ogg_page_serialno(og) == serialno)
	{
		sw->file_pagepos_found = sw->file_pagepos - og->header_len - og->body_len;
		return 1;
	}
	while (ogg_get_next_page(sw, serialno, og))
		if (ogg_page_serialno(og) == serialno)
		{
			sw->file_pagepos_found = sw->file_pagepos - og->header_len - og->body_len;
			return 1;
		}
	return 0;
}

// Returns:
// >= 0, number of sample within a page
// <  0 error
int FileOGG::ogg_get_page_of_sample(sync_window_t *sw, long serialno, ogg_page *og, samplenum sample)
{
// First make an educated guess about position
	if (sample >= asset->audio_length + start_sample)
	{
		errorbox("Illegal seek beyond end of samples\n");
		return 0;
	}
	off_t educated_guess = filedata_begin + (file_length - filedata_begin) * (sample - start_sample) / asset->audio_length - READ_SIZE;
	if (educated_guess < 0) 
		educated_guess = 0;
// now see if we won
	read_buffer_at(stream, sw, READ_SIZE, educated_guess);
	ogg_sync_and_get_next_page(sw, serialno, og);
	int64_t end_sample = ogg_page_granulepos(og);
	// linear seek to the sample
	int64_t start_sample = 0;
// TODO: Use bisection also
	if (end_sample <= sample)
	{
	// scan forward
		while (end_sample <= sample)
		{
			ogg_get_next_page(sw, serialno, og); 
			start_sample = end_sample;
			end_sample = ogg_page_granulepos(og);
		}
		ogg_get_prev_page(sw, serialno, og);
		while (ogg_page_continued(og) && ogg_page_packets(og) == 1)
			ogg_get_prev_page(sw, serialno, og);
	} else
	{
	// scan backward
		start_sample = end_sample;
		while (start_sample > sample || (ogg_page_continued(og) && 
			ogg_page_packets(og) == 1))
		{
			ogg_get_prev_page(sw, serialno, og);
			end_sample = start_sample;
			start_sample = ogg_page_granulepos(og);
		}
	// go forward one page at the end

	}
	return 1;
}

// seeks, so next sample returned will be the correct one
// sample here is still the vorbis sample number (= cinelerra sample number + start_sample)
// reinits the decoding engine

int FileOGG::ogg_seek_to_sample(sync_window_t *sw, long serialno, samplenum sample)
{
	// MAYBE FIXME - find out if we really are decoding previous two packets or not
	// get to the sample
	ogg_page og;
	ogg_packet op;
	if (!ogg_get_page_of_sample(sw, serialno, &og, sample))
	{
		errorbox("Seeking to sample's page failed\n");
		return 0;
	}

	vorbis_synthesis_restart(&tf->vd);
	ogg_stream_reset(&tf->vo);
	ogg_stream_pagein(&tf->vo, &og);
	int sync = 0;

	int64_t current_comming_sample = -1;
	while (1) 
	{
		// make sure we have a packet ready
		while (ogg_stream_packetpeek(&tf->vo, NULL) != 1)
		{
			if (!ogg_get_next_page(sw, serialno, &og))
			{
				errorbox("Cannot find next page while seeking\n");
				return 0;
			}
			ogg_stream_pagein(&tf->vo, &og);
		}
		ogg_stream_packetout(&tf->vo, &op);
		if (sync)
		{
			
			if(!vorbis_synthesis(&tf->vb, &op))
			{
				int ret= vorbis_synthesis_blockin(&tf->vd, &tf->vb);
				int64_t previous_comming_sample = current_comming_sample;
				current_comming_sample += vorbis_synthesis_pcmout(&tf->vd, NULL);
				if (current_comming_sample > sample)
				{
					if (previous_comming_sample > sample)
					{
						errorbox("Ogg decoding error while seeking sample\n");
					}
					vorbis_synthesis_read(&tf->vd, (sample - previous_comming_sample));
					return 1; // YAY next sample read is going to be ours, sexy!
				} else
				{
					// discard decoded data before current sample
					vorbis_synthesis_read(&tf->vd, (current_comming_sample - previous_comming_sample));
					
				}
			}
		}
		if (!sync && op.granulepos >= 0)
		{
			sync = 1;
			current_comming_sample = op.granulepos;
			if(!vorbis_synthesis(&tf->vb, &op))
			{
				vorbis_synthesis_blockin(&tf->vd, &tf->vb);
				if (vorbis_synthesis_pcmout(&tf->vd, NULL) != 0)
				{
					errorbox("Something wrong while trying to seek\n");
					return 0;
				}
			
			}
			
		}
	}
	return 0;
}

int FileOGG::ogg_get_page_of_frame(sync_window_t *sw, long serialno, ogg_page *og, framenum frame)
{
	if (frame >= asset->video_length + start_frame)
	{
		errorbox("Illegal seek beyond end of frames in OGG file");
		return 0;
	}
	off_t educated_guess = filedata_begin + (file_length - filedata_begin) * (frame - start_frame) / asset->video_length - READ_SIZE/2;
	if (educated_guess > file_length - READ_SIZE)
		educated_guess = file_length - READ_SIZE;
	if (educated_guess < filedata_begin) 
		educated_guess = filedata_begin;

// now see if we won
	read_buffer_at(stream, sw, READ_SIZE, educated_guess);
	ogg_sync_and_get_next_page(sw, serialno, og);
	int64_t pageend_frame;
	int read_back = 0;
	// find the page with "real" ending
	while ((pageend_frame = ogg_page_granulepos(og)) == -1)
	{
		if (ogg_get_next_page(sw, serialno, og) == 0) 
		{
			read_back = 1;
			break;
		} 
	}
	pageend_frame = theora_granule_frame(&tf->td, ogg_page_granulepos(og));

	// FIXME - MEDIUM PRIORITY: read back if we've gone too far and no page of our serialno at all can be found

	// linear seek to the sample
	int64_t start_frame = 0;
// TODO: Use bisection also
	int discard_packets = 0;
	int missp = 0;
	int missm = 0;
	if (pageend_frame <= frame)
	{
	// scan forward
		while (pageend_frame < frame)
		{
			do {
				ogg_get_next_page(sw, serialno, og); 
			} while (ogg_page_packets(og) == 0);
			pageend_frame = theora_granule_frame(&tf->td, ogg_page_granulepos(og));
			missp++;
		}
		// go back if this frame starts on previous page
		if (ogg_page_continued(og) && pageend_frame - ogg_page_packets(og) == frame - 1)
		{
			do {
				ogg_get_prev_page(sw, serialno, og); 
			} while (ogg_page_packets(og) == 0 && ogg_page_continued(og));
		}
		pageend_frame = theora_granule_frame(&tf->td, ogg_page_granulepos(og));
	} else
	{
	// scan backward
		int64_t first_frame_on_page = theora_granule_frame(&tf->td, ogg_page_granulepos(og)) - ogg_page_packets(og) + 2;
		if (!ogg_page_continued(og))
			first_frame_on_page--;
		while (first_frame_on_page > frame)
		{
			do {
				ogg_get_prev_page(sw, serialno, og); 
			} while (ogg_page_packets(og) == 0 && ogg_page_continued(og));
			missm++;
			first_frame_on_page = theora_granule_frame(&tf->td, ogg_page_granulepos(og)) - ogg_page_packets(og) + 2;
			if (!ogg_page_continued(og))
				first_frame_on_page--;
		}
	}
	return 1;
}


int FileOGG::ogg_seek_to_keyframe(sync_window_t *sw, long serialno, framenum frame, framenum *keyframe_number)
{
	ogg_page og;
	ogg_packet op;
	if (!ogg_get_page_of_frame(sw, serialno, &og, frame))
	{
		errorbox("Seeking to OGG frame failed\n");
		return 0;
	}
	// TODO: if the frame we are looking for continoues on the next page, we don't need to do this
	// Now go to previous page in order to find out the granulepos
	// Don't do it in case this is the first page.
	framenum granulepos, iframe, pframe;
	granulepos = ogg_page_granulepos(&og);
	iframe = granulepos >> theora_keyframe_granule_shift;
	pframe = granulepos - (iframe << theora_keyframe_granule_shift);
	// check if maybe iframe is known from this page already
	if (granulepos != -1 && iframe <= frame)
	{
		// optimisation, iframe is already known from this page
	} else
	{
		// get previous page so we will get the iframe number 
		do {
			ogg_get_prev_page(sw, serialno, &og); 
		} while (ogg_page_packets(&og) == 0);

		granulepos = ogg_page_granulepos(&og);
		iframe = granulepos >> theora_keyframe_granule_shift;
		pframe = granulepos - (iframe << theora_keyframe_granule_shift);
	}
	framenum first_frame_on_page = theora_granule_frame(&tf->td, ogg_page_granulepos(&og)) - ogg_page_packets(&og) + 2;
	if (!ogg_page_continued(&og))
		first_frame_on_page--;
	if (first_frame_on_page <= iframe)
	{
		// optimisation, happens mainly in low-bitrate streams, it spares us one seek
	} else
	{
		// get the page where keyframe starts
		if (!ogg_get_page_of_frame(sw, serialno, &og, iframe))
		{
			errorbox("Seeking to OGG frame failed\n");
			return 0;
		}
	}

	ogg_stream_reset(&tf->to);
	ogg_stream_pagein(&tf->to, &og);

	// Read until one frame before keyframe
	int numread = iframe - (theora_granule_frame(&tf->td, ogg_page_granulepos(&og)) - ogg_page_packets(&og)) - 1;
	if (ogg_page_continued(&og))
		numread--;

	while (numread > 0)
	{
		while (ogg_stream_packetpeek(&tf->to, NULL) != 1)
		{
			if (!ogg_get_next_page(sw, serialno, &og))
			{
				errorbox("Cannot find next page while seeking OGG\n");
				return 0;
			}
			ogg_stream_pagein(&tf->to, &og);
		}
		ogg_stream_packetout(&tf->to, &op);
		numread --;
	}
	*keyframe_number = iframe;
	return 1;
}


int FileOGG::check_sig(Asset *asset)
{

	FILE *fd = fopen(asset->path, "rb");

// Test for "OggS"
	fseek(fd, 0, SEEK_SET);
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
	if (wr)
	{
		if (final_write)
		{
			if (asset->audio_data)
				write_samples_vorbis(0, 0, 1); // set eos
			if (asset->video_data)
				write_frames_theora(0, 1, 1); // set eos
		}
		flush_ogg(1); // flush all

		if (asset->audio_data)
		{
			vorbis_block_clear (&tf->vb);
			vorbis_dsp_clear (&tf->vd);
			vorbis_info_clear (&tf->vi);
			ogg_stream_clear (&tf->vo);
		}
		if (asset->video_data)
		{
			theora_info_clear (&tf->ti);
			ogg_stream_clear (&tf->to);
			theora_clear (&tf->td);
		}

		if (stream) fclose(stream);
		stream = 0;
	} 
	else if (rd)
	{
		if (asset->audio_data)
		{
			vorbis_block_clear (&tf->vb);
			vorbis_dsp_clear (&tf->vd);
			vorbis_comment_clear (&tf->vc);
			vorbis_info_clear (&tf->vi);
			ogg_stream_clear (&tf->vo);
		}
		theora_comment_clear(&tf->tc);
		if (asset->video_data)
		{
			theora_info_clear (&tf->ti);
			theora_comment_clear (&tf->tc);
			theora_clear (&tf->td);
			ogg_stream_clear (&tf->to);
		}

		if (stream) fclose(stream);
		stream = 0;
	}
}

void FileOGG::close_file_derived()
{
	if (stream) fclose(stream);
	stream = 0;
}

void FileOGG::set_video_position(framenum x)
{
	next_frame_position = x + start_frame;
}

int FileOGG::colormodel_supported(int colormodel)
{
	if (colormodel == BC_YUV420P)
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
	if(!stream) return 1;

	// skip is cheaper than seek, do it...
	int decode_frames = 0;
	int expect_keyframe = 0;
	if (ogg_frame_position >= 0 && 
		next_frame_position >= ogg_frame_position && 
		next_frame_position - ogg_frame_position < 32)
	{
		decode_frames = next_frame_position - ogg_frame_position;
	} else
	if (next_frame_position != ogg_frame_position)
	{
		if (!ogg_seek_to_keyframe(tf->videosync, tf->to.serialno, next_frame_position, &ogg_frame_position))
		{
			errorbox("Error while seeking to frame's keyframe (frame: %i, keyframe: %i)", next_frame_position, ogg_frame_position);
			return 1;
		}
		// skip frames must be > 0 here
		decode_frames = next_frame_position - ogg_frame_position + 1; 
		ogg_frame_position --; // ogg_frame_position is at last decoded frame, so it will point right 
		if (decode_frames <= 0) 
		{
			errorbox("Error while seeking to keyframe, wrong keyframe number (frame: %i, keyframe: %i)", next_frame_position, ogg_frame_position);
			return 1;
		}
		expect_keyframe = 1;
	}

// THIS IS WHAT CAUSES SLOWNESS OF SEEKING, but we can do nothing about it.
	while (decode_frames > 0)
	{
		ogg_page og;
		ogg_packet op;
		while (ogg_stream_packetpeek(&tf->to, NULL) != 1)
		{
			if (!ogg_get_next_page(tf->videosync, tf->to.serialno, &og))
			{
				errorbox("Cannot find next page while OGG seeking");
				return 1;
			}
			ogg_stream_pagein(&tf->to, &og);
		}
		ogg_stream_packetout(&tf->to, &op);
		if (expect_keyframe && !theora_packet_iskeyframe(&op))
		{
			errorbox("Expecting a theora keyframe, but didn't get it");
		}
		expect_keyframe = 0;

		// decode
		theora_decode_packetin(&tf->td, &op);

		decode_frames --;
		ogg_frame_position ++;
	}
	{
		yuv_buffer yuv;
		int ret = theora_decode_YUVout (&tf->td, &yuv);
		if (ret)
		{
			errorbox("theora_decode_YUVout() failed with code %i", ret);
		}

// Dirty magic 
		VFrame *temp_frame = new VFrame(yuv.y, 
						0,
						yuv.u - yuv.y,
						yuv.v - yuv.y,
						- yuv.y_stride,
						yuv.y_height,
						BC_YUV420P,
						- yuv.y_stride);
		// copy into temp frame...

		cmodel_transfer(frame->get_rows(),
			temp_frame->get_rows(),
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			temp_frame->get_y(),
			temp_frame->get_u(),
			temp_frame->get_v(),
			0,
			0,
			yuv.y_width,
			yuv.y_height,
			0,
			0,
			yuv.y_width,  // temp_frame can be larger than frame if width not dividable by 16
			yuv.y_height,
			BC_YUV420P,
			frame->get_color_model(),
			0,
			-temp_frame->get_w(),
			frame->get_w());
		delete temp_frame;
	}

	next_frame_position ++;
	return 0;
}

int FileOGG::ogg_decode_more_samples(sync_window_t *sw, long serialno)
{
	ogg_page og;
	ogg_packet op;
	int done = 0;
	while (!done)
	{
		while (ogg_stream_packetpeek(&tf->vo, NULL) != 1)
		{
			if (!ogg_get_next_page(sw, serialno, &og))
			{
				errorbox("Cannot find next page while trying to decode more OGG samples");
				return 0;
			}
			ogg_stream_pagein(&tf->vo, &og);
		}
		ogg_stream_packetout(&tf->vo, &op);
		if(!vorbis_synthesis(&tf->vb, &op))
		{
			done = 1;
			vorbis_synthesis_blockin(&tf->vd, &tf->vb);
		}
	}
	return 1;
}

void FileOGG::set_audio_position(samplenum x)
{
	next_sample_position = x + start_sample;
}

int FileOGG::move_history(int from, int to, int len)
{
	for(int i = 0; i < asset->channels; i++)
		memmove(pcm_history[i] + to, pcm_history[i] + from, sizeof(float) * len);
	history_start = history_start + from - to;
	return 0;
} 

int FileOGG::read_samples(double *buffer, int len)
{
	float **vorbis_buffer;
	if (len <= 0) 
		return 0;

	if(len > HISTORY_MAX)
	{
		errorbox("Too many OGG samples (max samples=%d)\n", HISTORY_MAX);
		return 1;
	}

	if(!pcm_history)
	{
		pcm_history = new float*[asset->channels];
		for(int i = 0; i < asset->channels; i++)
			pcm_history[i] = new float[HISTORY_MAX];
		history_start = -100000000; // insane value to trigger reload
		history_size = 0;
	}

	int64_t hole_start = -1;
	int64_t hole_len = -1;
	int64_t hole_absstart = -1;
	int64_t hole_fill = 0;

	if (history_start < next_sample_position && history_start + history_size > next_sample_position && history_start + history_size < next_sample_position + len) 
	{
		hole_fill = 1;
		hole_start = history_start + history_size - next_sample_position;
		hole_len = history_size - hole_start;
		hole_absstart = next_sample_position + hole_start;
		move_history(next_sample_position - history_start,
				0,
				hole_start);
	} else
	if (next_sample_position < history_start && history_start < next_sample_position + len)
	{
		hole_fill = 1;
		hole_start = 0;
		hole_len = history_start - next_sample_position;
		hole_absstart = next_sample_position;
		move_history(0, 
				history_start - next_sample_position,
				history_size - history_start + next_sample_position);
	} else 
	if (next_sample_position >= history_start + history_size || next_sample_position + len <= history_start)
	{
		hole_fill = 1;
		hole_start = 0;
		hole_len = HISTORY_MAX;
		hole_absstart = next_sample_position;
		history_start = hole_absstart;
		history_size = hole_len;
	}

	if (hole_fill)
	{
		if (hole_start < 0 || hole_len <= 0 || hole_absstart < 0)
		{
			errormsg("Error at finding out which range to read from file");
			return 1;
		}

		if (hole_absstart + hole_len > asset->audio_length + start_sample)
		{
			hole_len = asset->audio_length + start_sample - hole_absstart;
			history_size = asset->audio_length + start_sample - history_start;
		}
		else
		{
			history_size = HISTORY_MAX;
		}

		int64_t samples_read = 0;
		if (ogg_sample_position != hole_absstart)
		{
			ogg_sample_position = hole_absstart;
			if (!ogg_seek_to_sample(tf->audiosync, tf->vo.serialno, ogg_sample_position))
			{
				errormsg("Error while seeking to OGG sample");
				return 1;
			}
		}
		// now we have ogg_sample_positon aligned
		int64_t samples_to_read = hole_len;
		while (samples_read < hole_len)
		{
			int64_t waiting_samples = vorbis_synthesis_pcmout(&tf->vd, &vorbis_buffer);
			int64_t takeout_samples;
			if (waiting_samples > samples_to_read - samples_read)
				takeout_samples = samples_to_read - samples_read;
			else 
				takeout_samples = waiting_samples;

			int i, j;
			for(int i = 0; i < asset->channels; i++)
			{
				float *input = vorbis_buffer[i];
				float *output = pcm_history[i] + hole_start;
				// TODO: use memcpy
				for(int j = 0; j < takeout_samples ; j++)
				{
					output[j] = input[j];
				}
			}

			vorbis_synthesis_read(&tf->vd, takeout_samples);
			samples_read += takeout_samples;
			ogg_sample_position += takeout_samples;
			hole_start += takeout_samples;

			if (samples_read < hole_len)
				if (!ogg_decode_more_samples(tf->audiosync, tf->vo.serialno))
				{
					ogg_sample_position = -1;
					return 1;
				}
		}
	}

	// now we can be sure our history is correct, just copy it out
	if (next_sample_position < history_start || next_sample_position + len > history_start + history_size)
	{
		errorbox("History not aligned properly\n\tnext_sample_position: %lli, length: %d\n\thistory_start: %lld, length: %lld",
				next_sample_position, len, history_start, history_size);
		return 1;
	}
	float *input = pcm_history[file->current_channel] + next_sample_position - history_start;
	for (int i = 0; i < len; i++)
		buffer[i] = input[i];

	next_sample_position += len;
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

void FileOGG::flush_ogg (int e_o_s)
{
	int len;
	ogg_page og;

	flush_lock->lock();
	/* flush out the ogg pages  */
	while(1) {
	/* Get pages for both streams, if not already present, and if available.*/
		if(asset->video_data && !tf->vpage_valid) {
			// this way seeking is much better,
			// not sure if 23 packets  is a good value. it works though
			int v_next=0;
			if(tf->v_pkg>22 && ogg_stream_flush(&tf->to, &og) > 0) {
				v_next=1;
			}
			else if(ogg_stream_pageout(&tf->to, &og) > 0) {
				v_next=1;
			}
			if(v_next) {
				len = og.header_len + og.body_len;
				if(tf->vpage_buffer_length < len) {
					tf->vpage = (unsigned char *)realloc(tf->vpage, len);
					tf->vpage_buffer_length = len;
				}
				tf->vpage_len = len;
				memcpy(tf->vpage, og.header, og.header_len);
				memcpy(tf->vpage+og.header_len , og.body, og.body_len);

				tf->vpage_valid = 1;
				tf->videotime = theora_granule_time (&tf->td,
				ogg_page_granulepos(&og));
			}
		}
		if(asset->audio_data && !tf->apage_valid) {
			// this way seeking is much better,
			// not sure if 23 packets  is a good value. it works though
			int a_next=0;
			if(tf->a_pkg>22 && ogg_stream_flush(&tf->vo, &og) > 0) {
				a_next=1;
			}
			else if(ogg_stream_pageout(&tf->vo, &og) > 0) {
				a_next=1;
			}
			if(a_next) {
				len = og.header_len + og.body_len;
				if(tf->apage_buffer_length < len) {
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

		if(!asset->audio_data && tf->vpage_valid) {
			write_video_page();
		}
		else if(!asset->video_data && tf->apage_valid) {
			write_audio_page();
		}
		/* We're using both. We can output only:
		 *  a) If we have valid pages for both
		 *  b) At EOS, for the remaining stream.
		 */
		else if(tf->vpage_valid && tf->apage_valid) {
			/* Make sure they're in the right order. */
			if(tf->videotime <= tf->audiotime)
				write_video_page();
			else
				write_audio_page();
		}
		else if(e_o_s && tf->vpage_valid) {
			write_video_page();
		}
		else if(e_o_s && tf->apage_valid) {
			write_audio_page();
		}
		else {
			break; /* Nothing more writable at the moment */
		}
	}
	flush_lock->unlock();
}


int FileOGG::write_samples_vorbis(double **buffer, int len, int e_o_s)
{
	int i,j, count = 0;
	float **vorbis_buffer;
	static int samples = 0;
	samples += len;
	if(e_o_s)
	{
		vorbis_analysis_wrote (&tf->vd, 0);
	} else
	{
		vorbis_buffer = vorbis_analysis_buffer (&tf->vd, len);
		/* double to float conversion */
		for(i = 0; i<asset->channels; i++)
		{
			for (j = 0; j < len; j++)
			{
				vorbis_buffer[i][j] = buffer[i][j];
			}
		}
		vorbis_analysis_wrote (&tf->vd, len);
	}
	while(vorbis_analysis_blockout (&tf->vd, &tf->vb) == 1)
	{
		/* analysis, assume we want to use bitrate management */
		vorbis_analysis (&tf->vb, NULL);
		vorbis_bitrate_addblock (&tf->vb);

		/* weld packets into the bitstream */
		while (vorbis_bitrate_flushpacket (&tf->vd, &tf->op))
		{
			flush_lock->lock();
			ogg_stream_packetin (&tf->vo, &tf->op);
			tf->a_pkg++;
			flush_lock->unlock();
		}
	}
	flush_ogg(0);
	return 0;
}

int FileOGG::write_samples(double **buffer, int len)
{
	if (len > 0)
		return write_samples_vorbis(buffer, len, 0);
	return 0;
}

int FileOGG::write_frames_theora(VFrame ***frames, int len, int e_o_s)
{
	// due to clumsy theora's design we need to delay writing out by one frame
	// always stay one frame behind, so we can correctly encode e_o_s

	int i, j, result = 0;

	if(!stream) return 0;

	for(j = 0; j < len && !result; j++)
	{
		if (temp_frame) // encode previous frame if available
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
			if (ret)
			{
				errormsg("Theora_encode_YUVin() failed with code %i\nyuv_buffer: y_width: %i, y_height: %i, y_stride: %i, uv_width: %i, uv_height: %i, uv_stride: %i",
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
		if (e_o_s) 
			break;

		if (!temp_frame)
		{
			temp_frame = new VFrame (0, 
						tf->ti.width, 
						tf->ti.height,
						BC_YUV420P);
		} 
		VFrame *frame = frames[0][j];
		int in_color_model = frame->get_color_model();
		if (in_color_model == BC_YUV422P &&
			temp_frame->get_w() == frame->get_w() &&
			temp_frame->get_h() == frame->get_h() &&
			temp_frame->get_bytes_per_line() == frame->get_bytes_per_line())
		{
			temp_frame->copy_from(frame);
		} else
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
	return 0;
}

int FileOGG::write_frames(VFrame ***frames, int len)
{
	return write_frames_theora(frames, len, 0);
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
	if (default_asset)
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
	if (default_asset->audio_data)
		total_packages++;
	if (default_asset->video_data)
		total_packages += preferences->renderfarm_job_count;

	packages = new RenderPackage*[total_packages];

	int local_current_package = 0;
	if (default_asset->audio_data)
	{
		packages[local_current_package] = new RenderPackage;
		sprintf(packages[current_package]->path, "%s.audio", default_asset->path);
		local_current_package++;
	}

	if (default_asset->video_data)
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
	if (current_package == total_packages)
		return 0;

	RenderPackage *result = 0;
	if (current_package == 0 && default_asset->audio_data)
	{
		result = packages[0];
		result->audio_start_pts = audio_start_pts;
		result->video_start_pts = video_start_pts;
		result->audio_end_pts = audio_end_pts;
		result->video_end_pts = video_end_pts;
		result->audio_do = 1;
		result->video_do = 0;
	} else if (default_asset->video_data)
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
		if (current_package == total_packages -1 ) 
		{
			result->audio_end_pts = audio_end_pts;
			result->video_end_pts = video_end_pts;
		}

		audio_pts = result->audio_end_pts;
		video_pts = result->video_end_pts;
	}

	current_package ++;
	return result;
}

void PackagingEngineOGG::get_package_paths(ArrayList<char*> *path_list)
{
	for(int i = 0; i < total_packages; i++)
	{
		path_list->append(strdup(packages[i]->path));
	}
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
	if (default_asset->audio_data)
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
		audio_file->ogg_seek_to_databegin(audio_file->tf->audiosync, audio_file->tf->vo.serialno);
	}

	if (default_asset->video_data)
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
		video_file->ogg_seek_to_databegin(video_file->tf->videosync, video_file->tf->to.serialno);
	}
// Output file
	File *output_file_gen = new File();
	output_file_gen->open_file(preferences,
		default_asset,
		0,
		1,
		default_asset->sample_rate, 
		default_asset->frame_rate);
	FileOGG *output_file = (FileOGG*) output_file_gen->file;

	ogg_page og;    /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet op;  /* one raw packet of data for decode */

	int audio_ready = default_asset->audio_data;
	int video_ready = default_asset->video_data;
	int64_t video_packetno = 1;
	int64_t audio_packetno = 1;
	int64_t frame_offset = 0;
	int64_t current_frame = 0;
	while ((default_asset->audio_data && audio_ready) || (default_asset->video_data && video_ready))
	{
		if (video_ready)
		{
			while (ogg_stream_packetpeek(&video_in_stream, NULL) != 1) // get as many pages as needed for one package
			{
				if (!video_file->ogg_get_next_page(video_file->tf->videosync, video_file->tf->to.serialno, &video_file->tf->videopage))
				{
					// We are at the end of our file, see if it is more and open more if there is
					if (local_current_package < total_packages)
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
						video_file = (FileOGG*) video_file_gen->file;
						ogg_stream_init(&video_in_stream, video_file->tf->to.serialno);
						int64_t fp   = 0;
						video_file->ogg_seek_to_databegin(video_file->tf->videosync, video_file->tf->to.serialno);

					} else
						video_ready = 0;
					break;
				}
				ogg_stream_pagein(&video_in_stream, &video_file->tf->videopage);
			}
			while (ogg_stream_packetpeek(&video_in_stream, NULL) == 1) // get all packets out of the page
			{
				ogg_stream_packetout(&video_in_stream, &op);
				if (local_current_package != total_packages) // keep it from closing the stream
					op.e_o_s = 0;
				if (video_packetno != 1)                     // if this is not the first video package do not start with b_o_s
					op.b_o_s = 0;
				else
					op.b_o_s = 1;
				op.packetno = video_packetno;
				video_packetno ++;
				int64_t granulepos = op.granulepos;
				if (granulepos != -1)
				{
				// Fix granulepos!
					int64_t rel_iframe = granulepos >> video_file->theora_keyframe_granule_shift;
					int64_t rel_pframe = granulepos - (rel_iframe << video_file->theora_keyframe_granule_shift);
					int64_t rel_current_frame = rel_iframe + rel_pframe;
					current_frame = frame_offset + rel_current_frame;
					int64_t abs_iframe = current_frame - rel_pframe;

					op.granulepos = (abs_iframe << video_file->theora_keyframe_granule_shift) + rel_pframe;
				}
				ogg_stream_packetin (&output_file->tf->to, &op);
				output_file->tf->v_pkg++; 
			}
		}
		if (audio_ready)
		{
			while (ogg_stream_packetpeek(&audio_in_stream, NULL) != 1) // get as many pages as needed for one package
			{
				if (!audio_file->ogg_get_next_page(audio_file->tf->audiosync, audio_file->tf->vo.serialno, &audio_file->tf->audiopage))
				{
					audio_ready = 0;
					break;
				}
				ogg_stream_pagein(&audio_in_stream, &audio_file->tf->audiopage);
			}
			while (ogg_stream_packetpeek(&audio_in_stream, NULL) == 1) // get all packets out of the page
			{
				ogg_stream_packetout(&audio_in_stream, &op);
				ogg_stream_packetin (&output_file->tf->vo, &op);
				audio_packetno++;
				output_file->tf->a_pkg++; 
			}
		}
		output_file->flush_ogg(0);
	}
// Just prevent thet write_samples and write_frames are called
	output_file->final_write = 0;

	if (default_asset->audio_data)
	{
		ogg_stream_clear(&audio_in_stream);
		audio_file_gen->close_file();
		delete audio_file_gen;
		Garbage::delete_object(audio_asset);
	}
	if (default_asset->video_data)
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
