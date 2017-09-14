
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
#include "cache.inc"
#include "condition.h"
#include "edit.h"
#include "file.h"
#include "fileavlibs.h"
#include "filedv.h"
#include "filebase.h"
#include "fileexr.h"
#include "filexml.h"
#include "filejpeg.h"
#include "filempeg.h"
#include "filepng.h"
#include "filesndfile.h"
#include "filetga.h"
#include "filethread.h"
#include "filetiff.h"
#include "fileyuv.h"
#include "formattools.h"
#include "framecache.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "pluginserver.h"
#include "resample.h"
#include "vframe.h"

File::File()
{
	cpus = 1;
	asset = new Asset;
	format_completion = new Mutex("File::format_completion");
	write_lock = new Condition(1, "File::write_lock");
	frame_cache = new FrameCache;
	reset_parameters();
}

File::~File()
{
	if(getting_options)
	{
		if(format_window) format_window->set_done(0);
		format_completion->lock("File::~File");
		format_completion->unlock();
	}

	if(temp_frame) delete temp_frame;

	close_file(0);
	Garbage::delete_object(asset);
	delete format_completion;
	delete write_lock;
	if(frame_cache) delete frame_cache;
}

void File::reset_parameters()
{
	file = 0;
	writing = 0;
	audio_thread = 0;
	video_thread = 0;
	getting_options = 0;
	format_window = 0;
	temp_frame = 0;
	resample = 0;
	resample_float = 0;
	use_cache = 0;
	preferences = 0;
	playback_subtitle = -1;
}

void File::raise_window()
{
	if(getting_options && format_window)
	{
		format_window->raise_window();
		format_window->flush();
	}
}

void File::close_window()
{
	if(getting_options)
	{
		format_window->set_done(1);
		getting_options = 0;
	}
}

void File::get_options(FormatTools *format, int options)
{
	BC_WindowBase *parent_window = format->window;
	Asset *asset = format->asset;

	getting_options = 1;
	format_completion->lock("File::get_options");
	switch(asset->format)
	{
		case FILE_RAWDV:
			FileDV::get_parameters(parent_window,
				asset,
				format_window,
				options);
			break;
		case FILE_PCM:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
			FileSndFile::get_parameters(parent_window, 
				asset,
				format_window,
				options);
			break;
		case FILE_AC3:
		case FILE_AVI:
		case FILE_MOV:
		case FILE_OGG:
		case FILE_WAV:
		case FILE_MP3:
			FileAVlibs::get_parameters(parent_window,
				asset,
				format_window,
				options);
			break;
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			FileJPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
#ifdef HAVE_OPENEXR
		case FILE_EXR:
		case FILE_EXR_LIST:
			FileEXR::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
#endif
		case FILE_YUV:
			FileYUV::get_parameters(parent_window,
				asset,
				format_window,
				options,
				format);
			break;
		case FILE_PNG:
		case FILE_PNG_LIST:
			FilePNG::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
		case FILE_TGA:
		case FILE_TGA_LIST:
			FileTGA::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
		case FILE_TIFF:
		case FILE_TIFF_LIST:
			FileTIFF::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;

		default:
			break;
	}

	if(!format_window)
	{
		const char *s;

		if(options & SUPPORTS_AUDIO)
			s = _("audio");
		else if(options & SUPPORTS_VIDEO)
			s = _("video");
		else
			s = "";
		errorbox(_("No suitable %s codec found."), s);
	}

	getting_options = 0;
	format_window = 0;
	format_completion->unlock();
}

void File::set_asset(Asset *asset)
{
	this->asset->copy_from(asset, 1);
}

void File::set_processors(int cpus)   // Set the number of cpus for certain codecs
{
	this->cpus = cpus;
}

void File::set_subtitle(int value)
{
	this->playback_subtitle = value;
}

void File::set_cache_frames(int value)
{
	use_cache = value;
}

int File::purge_cache()
{
	return frame_cache->delete_oldest();
}


int File::open_file(Preferences *preferences, 
	Asset *asset, 
	int rd, 
	int wr,
	int base_samplerate,
	float base_framerate)
{
	int probe_result, rs;
	this->preferences = preferences;
	this->asset->copy_from(asset, 1);
	file = 0;

	probe_result = 0;
	if(rd)
	{
		probe_result = FileAVlibs::probe_input(this->asset);

		if(probe_result < 0)
			return FILE_NOT_FOUND;
	}

	switch(this->asset->format)
	{
// get the format now
// If you add another format to case 0, you also need to add another case for the
// file format #define.
	case FILE_UNKNOWN:
		FILE *stream;
		if(!(stream = fopen(this->asset->path, "rb")))
		{
			return FILE_NOT_FOUND;
		}

		char test[16];
		rs = fread(test, 16, 1, stream) != 1;
		fclose(stream);

		if(rs)
			return FILE_NOT_FOUND;

		if(strncmp(test, "TOC ", 4) == 0)
		{
			errormsg(_("Can't open TOC files directly"));
			return FILE_NOT_FOUND;
		}
		if(FilePNG::check_sig(this->asset))
// PNG list
			file = new FilePNG(this->asset, this);
		else
		if(FileJPEG::check_sig(this->asset))
// JPEG list
			file = new FileJPEG(this->asset, this);
		else
#ifdef HAVE_OPENEXR
		if(FileEXR::check_sig(this->asset, test))
// EXR list
			file = new FileEXR(this->asset, this);
		else
#endif
		if(FileTGA::check_sig(this->asset))
// TGA list
			file = new FileTGA(this->asset, this);
		else
		if(FileTIFF::check_sig(this->asset))
// TIFF list
			file = new FileTIFF(this->asset, this);
		else
		if(test[0] == '<' && test[1] == 'E' && test[2] == 'D' && test[3] == 'L' && test[4] == '>' ||
			test[0] == '<' && test[1] == 'H' && test[2] == 'T' && test[3] == 'A' && test[4] == 'L' && test[5] == '>' ||
			test[0] == '<' && test[1] == '?' && test[2] == 'x' && test[3] == 'm' && test[4] == 'l')
// XML file
			return FILE_IS_XML;
		else
			return FILE_UNRECOGNIZED_CODEC;
		break;

// format already determined
	case FILE_PCM:
	case FILE_AU:
	case FILE_AIFF:
	case FILE_SND:
		file = new FileSndFile(this->asset, this);
		break;

	case FILE_PNG:
	case FILE_PNG_LIST:
		file = new FilePNG(this->asset, this);
		break;

	case FILE_JPEG:
	case FILE_JPEG_LIST:
		file = new FileJPEG(this->asset, this);
		break;
#ifdef HAVE_OPENEXR
	case FILE_EXR:
	case FILE_EXR_LIST:
		file = new FileEXR(this->asset, this);
		break;
#endif
	case FILE_YUV:
		file = new FileYUV(this->asset, this);
		break;

	case FILE_TGA_LIST:
	case FILE_TGA:
		file = new FileTGA(this->asset, this);
		break;

	case FILE_TIFF:
	case FILE_TIFF_LIST:
		file = new FileTIFF(this->asset, this);
		break;

	case FILE_AC3:
	case FILE_AVI:
	case FILE_MOV:
	case FILE_OGG:
	case FILE_MP3:
	case FILE_WAV:
	case FILE_FLAC:
	case FILE_MPEGTS:
		file = new FileAVlibs(this->asset, this);
		break;

	case FILE_MPEG:
		file = new FileMPEG(this->asset, this);
		break;

	case FILE_RAWDV:
		file = new FileDV(this->asset, this);
		break;

	default:
		return 1;
	}

// Reopen file with correct parser and get header.
	if(file->open_file(rd, wr))
	{
		delete file;
		file = 0;
	}

// Set extra writing parameters to mandatory settings.
	if(file && wr)
	{
		if(this->asset->dither) file->set_dither();
		writing = 1;
	}

// Synchronize header parameters
	if(file)
	{
		asset->copy_from(this->asset, 1);
	}
	if(file)
		return FILE_OK;
	else
		return FILE_NOT_FOUND;
}

void File::close_file(int ignore_thread)
{
	if(!ignore_thread)
	{
		stop_audio_thread();
		stop_video_thread();
	}

	if(file) 
	{
		file->close_file();
		delete file;
	}

	if(resample) delete resample;
	if(resample_float) delete resample_float;

	reset_parameters();
}


int File::get_index(const char *index_path)
{
	if(file)
	{
		return file->get_index(index_path);
	}
	return 1;
}

void File::start_audio_thread(int buffer_size, int ring_buffers)
{
	if(!audio_thread)
	{
		audio_thread = new FileThread(this, 1, 0);
		audio_thread->start_writing(buffer_size, 0, ring_buffers, 0);
	}
}

void File::start_video_thread(int buffer_size, 
	int color_model, 
	int ring_buffers, 
	int compressed)
{
	if(!video_thread)
	{
		video_thread = new FileThread(this, 0, 1);
		video_thread->start_writing(buffer_size, 
			color_model, 
			ring_buffers, 
			compressed);
	}
}

void File::start_video_decode_thread()
{
	if(!video_thread)
	{
		video_thread = new FileThread(this, 0, 1);
		video_thread->start_reading();
	}
}

void File::stop_audio_thread()
{
	if(audio_thread)
	{
		audio_thread->stop_writing();
		delete audio_thread;
		audio_thread = 0;
	}
}

void File::stop_video_thread()
{
	if(video_thread)
	{
		video_thread->stop_reading();
		video_thread->stop_writing();
		delete video_thread;
		video_thread = 0;
	}
}

FileThread* File::get_video_thread()
{
	return video_thread;
}

samplenum File::get_audio_length(int base_samplerate) 
{
	samplenum result = asset->audio_length;
	if(result > 0)
	{
		if(base_samplerate > 0)
			return (samplenum)((double)result / asset->sample_rate * base_samplerate + 0.5);
		else
			return result;
	}
	else
		return -1;
}

framenum File::get_video_length(float base_framerate)
{ 
	framenum result = asset->video_length;
	if(result > 0)
	{
		if(base_framerate > 0)
			return (framenum)((double)result / asset->frame_rate * base_framerate + 0.5); 
		else
			return result;
	}
	else
		return -1;  // infinity
}

ptstime File::get_video_ptslen(void)
{
	return (ptstime)asset->video_length / asset->frame_rate;
}

// No resampling here.
int File::write_aframes(AFrame **frames)
{
	int result;

	if(file)
	{
		write_lock->lock("File::write_aframes");
		for(int i = 0; i < asset->channels; i++)
		{
			if(frames[i])
			{
				asset->audio_length += frames[i]->length;
				break;
			}
		}
		result = file->write_aframes(frames);
		write_lock->unlock();
		return result;
	}
	return 1;
}

// Can't put any cmodel abstraction here because the filebase couldn't be
// parallel.
int File::write_frames(VFrame ***frames, int len)
{
// Store the counters in temps so the filebase can choose to overwrite them.
	int result;

	int video_length_temp = asset->video_length;

	write_lock->lock("File::write_frames");

	result = file->write_frames(frames, len);

	asset->video_length = video_length_temp + len;
	write_lock->unlock();

	return result;
}

int File::write_audio_buffer(int len)
{
	int result = 0;
	if(audio_thread)
	{
		result = audio_thread->write_buffer(len);
	}
	return result;
}

int File::write_video_buffer(int len)
{
	int result = 0;
	if(video_thread)
	{
		result = video_thread->write_buffer(len);
	}

	return result;
}

AFrame** File::get_audio_buffer()
{
	if(audio_thread) return audio_thread->get_audio_buffer();
	return 0;
}

VFrame*** File::get_video_buffer()
{
	if(video_thread) return video_thread->get_video_buffer();
	return 0;
}

int File::get_samples(AFrame *aframe)
{
	int result;
	int samples;

	if(!file)
		return 0;

	if(aframe->samplerate == 0)
		aframe->samplerate = asset->sample_rate;
	if(aframe->buffer_length <= 0)
		return 0;
	if(aframe->source_duration <= 0)
	{
		if(aframe->source_length > 0)
			aframe->source_duration = (ptstime)aframe->source_length / aframe->samplerate;
		else
			return 0;
	} else
		aframe->source_length = round(aframe->source_duration * aframe->samplerate);
	aframe->position = round(aframe->source_pts * aframe->samplerate);
	if(aframe->length + aframe->source_length > aframe->buffer_length)
		aframe->source_length = aframe->buffer_length - aframe->length;
	if(aframe->source_length <= 0)
		return 0;

	// Resample
	if(aframe->samplerate != asset->sample_rate)
	{
		if(!file->prefer_samples_float())
		{
			if(!resample)
				resample = new Resample(this, asset->channels);

			samples = resample->resample(aframe->buffer,
					aframe->source_length,
					asset->sample_rate,
					aframe->samplerate,
					aframe->channel,
					aframe->position);
		}
		else
		{
			if(!resample_float)
				resample_float = new Resample_float(this, asset->channels);

			samples = resample_float->resample(aframe->buffer,
					aframe->source_length,
					asset->sample_rate,
					aframe->samplerate,
					aframe->channel,
					aframe->position);
		}
		aframe->source_length = samples;
		aframe->set_filled_length();
	}
	else
// Load directly
	{
// Never try to read more samples than exist in the file
		if(aframe->position + aframe->source_length > asset->audio_length) 
			aframe->source_length = asset->audio_length - aframe->position;

		result = file->read_aframe(aframe);
	}
	return result;
}

int File::get_frame(VFrame *frame, int is_thread)
{
	if(video_thread && !is_thread)
		return video_thread->read_frame(frame);

	if(file)
	{
		int supported_colormodel = colormodel_supported(frame->get_color_model());
// Test cache
		if(use_cache && frame_cache->get_frame(frame))
			return 0;
// Need temp
		if(frame->get_color_model() != BC_COMPRESSED &&
			!file->converts_frame() &&
			(supported_colormodel != frame->get_color_model() ||
			frame->get_w() != asset->width ||
			frame->get_h() != asset->height))
		{
			if(temp_frame)
			{
				if(!temp_frame->params_match(asset->width, asset->height, supported_colormodel))
				{
					delete temp_frame;
					temp_frame = 0;
				}
			}

			if(!temp_frame)
			{
				temp_frame = new VFrame(0,
					asset->width,
					asset->height,
					supported_colormodel);
			}
			temp_frame->copy_pts(frame);
			file->read_frame(temp_frame);
			frame->transfer_from(temp_frame);
			frame->copy_pts(temp_frame);
		}
		else
			file->read_frame(frame);

		if(use_cache)
			frame_cache->put_frame(frame, 1);
		return 0;
	}
	else
		return 1;
}

int File::supports(int format)
{
	switch(format)
	{
	case FILE_JPEG:
	case FILE_PNG:
	case FILE_TIFF:
#ifdef HAVE_OPENEXR
	case FILE_EXR:
#endif
	case FILE_TGA:
		return SUPPORTS_VIDEO | SUPPORTS_STILL;

	case FILE_JPEG_LIST:
	case FILE_PNG_LIST:
	case FILE_TIFF_LIST:
#ifdef HAVE_OPENEXR
	case FILE_EXR_LIST:
#endif
	case FILE_TGA_LIST:
		return SUPPORTS_VIDEO;

	case FILE_AIFF:
	case FILE_PCM:
	case FILE_AU:
	case FILE_SND:
		return SUPPORTS_AUDIO;

	case FILE_YUV:
		return FileYUV::supports(format);

	case FILE_RAWDV:
		return FileDV::supports(format);

	case FILE_AC3:
	case FILE_AVI:
	case FILE_MOV:
	case FILE_OGG:
	case FILE_MP3:
	case FILE_WAV:
		return FileAVlibs::supports(format, 0);
		break;
	}
	return (SUPPORTS_AUDIO | SUPPORTS_VIDEO);
}

int File::get_best_colormodel(int driver)
{
	return get_best_colormodel(asset, driver);
}

int File::get_best_colormodel(Asset *asset, int driver)
{
	switch(asset->format)
	{
	case FILE_RAWDV:
		return FileDV::get_best_colormodel(asset, driver);

	case FILE_MPEG:
		return FileMPEG::get_best_colormodel(asset, driver);

	case FILE_JPEG:
	case FILE_JPEG_LIST:
		return FileJPEG::get_best_colormodel(asset, driver);
#ifdef HAVE_OPENEXR
	case FILE_EXR:
	case FILE_EXR_LIST:
		return FileEXR::get_best_colormodel(asset, driver);
#endif
	case FILE_YUV:
		return FileYUV::get_best_colormodel(asset, driver);

	case FILE_PNG:
	case FILE_PNG_LIST:
		return FilePNG::get_best_colormodel(asset, driver);

	case FILE_TGA:
	case FILE_TGA_LIST:
		return FileTGA::get_best_colormodel(asset, driver);
	}

	return BC_RGB888;
}


int File::colormodel_supported(int colormodel)
{
	if(file)
		return file->colormodel_supported(colormodel);

	return BC_RGB888;
}

size_t File::get_memory_usage() 
{
	size_t result = 0;
	if(temp_frame) result += temp_frame->get_data_size();
	if(file) result += file->get_memory_usage();
	result += frame_cache->get_memory_usage();
	if(video_thread) result += video_thread->get_memory_usage();

	if(result < MIN_CACHEITEM_SIZE) result = MIN_CACHEITEM_SIZE;
	return result;
}

FrameCache* File::get_frame_cache()
{
	return frame_cache;
}
