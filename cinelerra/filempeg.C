
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

#include "aframe.h"
#include "asset.h"
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "byteorder.h"
#include "clip.h"
#include "condition.h"
#include "edit.h"
#include "file.h"
#include "filempeg.h"
#include "filesystem.h"
#include "indexfile.h"
#include "interlacemodes.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "preferences.h"
#include "vframe.h"
#include "videodevice.inc"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MPEG_YUV420 0
#define MPEG_YUV422 1


FileMPEG::FileMPEG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	fd = 0;
// May also be VMPEG or AMPEG if write status.
	if(asset->format == FILE_UNKNOWN) asset->format = FILE_MPEG;
	asset->byte_order = 0;
	temp_float = 0;
	temp_float_allocated = 0;
}

FileMPEG::~FileMPEG()
{
	close_file();
	if(temp_float)
		delete [] temp_float;
}

int FileMPEG::check_sig(Asset *asset)
{
	return mpeg3_check_sig(asset->path);
}


// Just create the Quicktime objects since this routine is also called
// for reopening.
int FileMPEG::open_file(int rd, int wr)
{
	int result = 0;

	if(rd)
	{
		int error = 0;
		if(!(fd = mpeg3_open(asset->path, &error)))
		{
			char string[BCTEXTLEN];
			if(error == MPEG3_INVALID_TOC_VERSION)
			{
				errormsg("Couldn't open %s because it has an invalid table of contents version.\n"
					"Rebuild the table of contents with mpeg3toc.",
					asset->path);
			}
			else
			if(error == MPEG3_TOC_DATE_MISMATCH)
			{
				errormsg("Couldn't open %s because the table of contents date differs from the source date.\n"
					"Rebuild the table of contents with mpeg3toc.",
					asset->path);
			}
			result = 1;
		}
		else
		{
// Determine if the file needs a table of contents and create one if needed.
// If it has video it must be scanned since video has keyframes.
			if(mpeg3_total_vstreams(fd))
			{
				if(create_index()) return 1;
			}

			mpeg3_set_cpus(fd, file->cpus);

			asset->audio_data = mpeg3_has_audio(fd);
			if(asset->audio_data)
			{
				asset->astreams = mpeg3_total_astreams(fd);
				asset->channels = mpeg3_audio_channels(fd, 0);
				for(int i = 0; i < asset->astreams; i++)
				{
					asset->astream_channels[i] = mpeg3_audio_channels(fd, i);
				}
				if(!asset->sample_rate)
					asset->sample_rate = mpeg3_sample_rate(fd, 0);
				asset->audio_length = mpeg3_audio_samples(fd, 0); 
				strncpy(asset->acodec, mpeg3_audio_format(fd, 0), sizeof(asset->acodec));
			}

			asset->video_data = mpeg3_has_video(fd);
			if(asset->video_data)
			{
				int i;
				asset->layers = mpeg3_total_vstreams(fd);
				asset->width = mpeg3_video_width(fd, 0);
				asset->height = mpeg3_video_height(fd, 0);
				asset->interlace_mode = BC_ILACE_MODE_UNDETECTED; 
				if((i = mpeg3_video_interlacemode(fd, 0)) >= 0)
				{
					switch(i)
					{
					case MPEG3_PROGRESSIVE:
						i = BC_ILACE_MODE_NOTINTERLACED;
						break;
					case MPEG3_BOTTOMFIRST:
						i = BC_ILACE_MODE_BOTTOM_FIRST;
						break;
					case MPEG3_TOPFIRST:
						i = BC_ILACE_MODE_TOP_FIRST;
						break;
					default:
						i = BC_ILACE_MODE_UNDETECTED;
						break;
					}
					asset->interlace_mode = i;
				}
				asset->video_length = mpeg3_video_frames(fd, 0);
				asset->vmpeg_cmodel = 
					(mpeg3_colormodel(fd, 0) == MPEG3_YUV422P) ? MPEG_YUV422 : MPEG_YUV420;
				if(!asset->frame_rate)
					asset->frame_rate = mpeg3_frame_rate(fd, 0);
				if(asset->aspect_ratio < 0)
					asset->aspect_ratio = mpeg3_aspect_ratio(fd, 0);

				asset->file_length = mpeg3_get_bytes(fd);
				asset->subtitles = mpeg3_subtitle_tracks(fd);

// Enable subtitles
				mpeg3_show_subtitle(fd, file->playback_subtitle);
			}
		}
	}

	if(wr)
	{
		errorbox("(VA)MPEG encoding is disabled for now");
		return 1;
	}
	return result;
}


int FileMPEG::create_index()
{
// Calculate TOC path
	char index_filename[BCTEXTLEN];
	char source_filename[BCTEXTLEN];
	IndexFile::get_index_filename(source_filename, 
		file->preferences->index_directory, 
		index_filename, 
		asset->path);
	char *ptr = strrchr(index_filename, '.');
	int error = 0;

	if(!ptr) return 1;

// File is a table of contents.
	if(fd && mpeg3_has_toc(fd)) return 0;

	sprintf(ptr, ".toc");

	mpeg3_t *fd_toc;

// Test existing copy of TOC
	mpeg3_close(fd);     // Always free old fd
	if((fd_toc = mpeg3_open(index_filename, &error)))
	{
// Just exchange old fd 
		fd = fd_toc;
	}
	else
	{
// Create progress window.
// This gets around the fact that MWindowGUI is locked.
		char progress_title[BCTEXTLEN];
		char string[BCTEXTLEN];
		sprintf(progress_title, "Creating %s\n", index_filename);
		int64_t total_bytes;
		mpeg3_t *index_file = mpeg3_start_toc(asset->path, index_filename, &total_bytes);
		struct timeval new_time;
		struct timeval prev_time;
		struct timeval start_time;
		struct timeval current_time;
		gettimeofday(&prev_time, 0);
		gettimeofday(&start_time, 0);

		BC_ProgressBox *progress = new BC_ProgressBox(-1, 
			-1, 
			progress_title, 
			total_bytes);
		progress->start();
		int result = 0;
		while(1)
		{
			int64_t bytes_processed;
			mpeg3_do_toc(index_file, &bytes_processed);
			gettimeofday(&new_time, 0);

			if(new_time.tv_sec - prev_time.tv_sec >= 1)
			{
				gettimeofday(&current_time, 0);
				int64_t elapsed_seconds = current_time.tv_sec - start_time.tv_sec;
				int64_t total_seconds = elapsed_seconds * total_bytes / bytes_processed;
				int64_t eta = total_seconds - elapsed_seconds;
				progress->update(bytes_processed, 1);
				sprintf(string, 
					"%sETA: %" PRId64 "m%" PRId64 "s",
					progress_title,
					eta / 60,
					eta % 60);
				progress->update_title(string, 1);
				prev_time = new_time;
			}

			if(bytes_processed >= total_bytes) break;
			if(progress->is_cancelled())
			{
				result = 1;
				break;
			}
		}

		mpeg3_stop_toc(index_file);

		progress->stop_progress();
		delete progress;

// Remove if error
		if(result)
		{
			remove(index_filename);
			return 1;
		}

// Reopen file from index path instead of asset path.
		if(!(fd = mpeg3_open(index_filename, &error)))
			return 1;
	}

	return 0;
}

void FileMPEG::close_file()
{
	if(fd)
	{
		mpeg3_close(fd);
		fd = 0;
	}
}

int FileMPEG::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver)
	{
	case PLAYBACK_X11:
		return BC_RGB888;

	case PLAYBACK_X11_XV:
	case PLAYBACK_ASYNCHRONOUS:
		if(asset->vmpeg_cmodel == MPEG_YUV420) return BC_YUV420P;
		if(asset->vmpeg_cmodel == MPEG_YUV422) return BC_YUV422P;
		break;

	case PLAYBACK_X11_GL:
		return BC_YUV888;

	case VIDEO4LINUX:
	case VIDEO4LINUX2:
		if(asset->vmpeg_cmodel == MPEG_YUV420) return BC_YUV420P;
		if(asset->vmpeg_cmodel == MPEG_YUV422) return BC_YUV422P;
		break;
	}
	return BC_RGB888;
}

int FileMPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileMPEG::get_index(const char *index_path)
{
	if(!fd) return 1;

// Convert the index tables from tracks to channels.
	if(mpeg3_index_tracks(fd))
	{
// Calculate size of buffer needed for all channels
		int buffer_size = 0;
		for(int i = 0; i < mpeg3_index_tracks(fd); i++)
		{
			buffer_size += mpeg3_index_size(fd, i) *
				mpeg3_index_channels(fd, i) *
				2;
		}

		asset->index_buffer = new float[buffer_size];

// Size of index buffer in floats
		int current_offset = 0;
// Current asset channel
		int current_channel = 0;
		asset->index_zoom = mpeg3_index_zoom(fd);

		for(int i = 0; i < mpeg3_index_tracks(fd); i++)
		{
			for(int j = 0; j < mpeg3_index_channels(fd, i); j++)
			{
				asset->index_offsets[current_channel] = current_offset;
				asset->index_sizes[current_channel] = mpeg3_index_size(fd, i) * 2;
				memcpy(asset->index_buffer + current_offset,
					mpeg3_index_data(fd, i, j),
					mpeg3_index_size(fd, i) * sizeof(float) * 2);

				current_offset += mpeg3_index_size(fd, i) * 2;
				current_channel++;
			}
		}

		FileSystem fs;
		asset->index_bytes = fs.get_size(asset->path);

		asset->write_index(index_path, buffer_size * sizeof(float));
		delete [] asset->index_buffer;

		return 0;
	}

	return 1;
}

size_t FileMPEG::get_memory_usage()
{
	if(fd)
		return mpeg3_memory_usage(fd);
	return 0;
}

int FileMPEG::to_mpeg_colormodel(int cmodel)
{
	switch(cmodel)
	{
	case BC_RGB565:
		return MPEG3_RGB565;
	case BC_BGR888:
		return MPEG3_BGR888;
	case BC_BGR8888:
		return MPEG3_BGRA8888;
	case BC_RGB888:
		return MPEG3_RGB888;
	case BC_RGBA8888:
		return MPEG3_RGBA8888;
	case BC_RGBA16161616:
		return MPEG3_RGBA16161616;
	case BC_YUV420P:
		return MPEG3_YUV420P;
	case BC_YUV422P:
		return MPEG3_YUV422P;
	default:
		return 0;
	}
}

int FileMPEG::read_frame(VFrame *frame)
{
	if(!fd) return 1;

	int result = 0;
	int src_cmodel;
	framenum current_frame = (frame->get_source_pts() + FRAME_OVERLAP) * asset->frame_rate;
	int layer = frame->get_layer();

	if(current_frame >= 0 && current_frame < asset->video_length)
		mpeg3_set_frame(fd, current_frame, layer);

	if(mpeg3_colormodel(fd, 0) == MPEG3_YUV420P)
		src_cmodel = BC_YUV420P;
	else
	if(mpeg3_colormodel(fd, 0) == MPEG3_YUV422P)
		src_cmodel = BC_YUV422P;

	switch(frame->get_color_model())
	{
	case BC_RGB565:
	case BC_BGR888:
	case BC_BGR8888:
	case BC_RGB888:
	case BC_RGBA8888:
	case BC_RGBA16161616:
		mpeg3_read_frame(fd, 
			frame->get_rows(), /* Array of pointers to the start of each output row */
			0,                    /* Location in input frame to take picture */
			0, 
			asset->width,
			asset->height,
			asset->width,                   /* Dimensions of output_rows */
			asset->height,
			to_mpeg_colormodel(frame->get_color_model()),
			layer);
			break;

// Use Temp
	default:
// Read these directly
		if(frame->get_color_model() == src_cmodel)
		{
			mpeg3_read_yuvframe(fd,
				(char*)frame->get_y(),
				(char*)frame->get_u(),
				(char*)frame->get_v(),
				0,
				0,
				asset->width,
				asset->height,
				layer);
		}
		else
// Process through temp frame
		{
			char *y, *u, *v;
			mpeg3_read_yuvframe_ptr(fd,
				&y,
				&u,
				&v,
				layer);

			if(y && u && v)
			{
				ColorModels::transfer_sws(frame->get_data(),
					0,
					frame->get_y(),
					frame->get_u(),
					frame->get_v(),
					(unsigned char*)y,
					(unsigned char*)u,
					(unsigned char*)v,
					asset->width,
					asset->height,
					asset->width,
					asset->height,
					src_cmodel,
					frame->get_color_model(),
					(asset->width + 15) & ~15,
					frame->get_bytes_per_line());
			}
		}
		break;
	}
	frame->set_source_pts((ptstime)current_frame / asset->frame_rate);
	frame->set_duration(1. / asset->frame_rate);
	frame->set_frame_number(current_frame);
	return 0;

noframe:
	frame->clear_frame();
	frame->set_source_pts((ptstime)current_frame / asset->frame_rate);
	frame->set_duration(1./asset->frame_rate);
	frame->set_frame_number(current_frame);
	return 1;
}

int FileMPEG::read_aframe(AFrame *aframe)
{
	int len;
	float *buf;

	if(!fd) return 0;
	len = aframe->source_length;

	if(aframe->float_buffer)
		buf = &aframe->float_buffer[aframe->length];
	else
	{
		if(temp_float_allocated < len)
		{
			if(temp_float)
				delete [] temp_float;
			temp_float = new float[len];
			temp_float_allocated = len;
		}
		buf = temp_float;
	}

	mpeg3_set_sample(fd, 
		aframe->position,
		asset->current_astream);

	mpeg3_read_audio(fd, 
		buf,      /* Pointer to pre-allocated buffer of floats */
		0,      /* Pointer to pre-allocated buffer of int16's */
		aframe->channel,          /* Channel to decode */
		len,         /* Number of samples to decode */
		asset->current_astream);    /* Stream containing the channel */
	if(aframe->buffer)
	{
		double *buffer = &aframe->buffer[aframe->length];
		for(int i = 0; i < len; i++)
			buffer[i] = temp_float[i];
	}
	aframe->set_filled_length();

	return 0;
}

int FileMPEG::prefer_samples_float()
{
	return 1;
}

const char* FileMPEG::strtocompression(const char *string)
{
	return "";
}

const char* FileMPEG::compressiontostr(const char *string)
{
	return "";
}

