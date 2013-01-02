
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
#include "fileac3.h"
#include "filedv.h"
#include "filebase.h"
#include "fileexr.h"
#include "filexml.h"
#include "filejpeg.h"
#include "filemov.h"
#include "filempeg.h"
#include "fileogg.h"
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
#include "stringfile.h"
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
		format_window->lock_window("File::close_window");
		format_window->set_done(1);
		format_window->unlock_window();
		getting_options = 0;
	}
}

void File::get_options(FormatTools *format, int options)
{
	BC_WindowBase *parent_window = format->window;
	ArrayList<PluginServer*> *plugindb = format->plugindb;
	Asset *asset = format->asset;

	getting_options = 1;
	format_completion->lock("File::get_options");
	switch(asset->format)
	{
		case FILE_AC3:
			FileAC3::get_parameters(parent_window,
				asset,
				format_window,
				options);
			break;
		case FILE_RAWDV:
			FileDV::get_parameters(parent_window,
				asset,
				format_window,
				options);
			break;
		case FILE_PCM:
		case FILE_WAV:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
			FileSndFile::get_parameters(parent_window, 
				asset,
				format_window,
				options);
			break;
		case FILE_MOV:
			FileMOV::get_parameters(parent_window, 
				asset,
				format_window,
				options,
				format->locked_compressor);
			break;
		case FILE_AMPEG:
		case FILE_VMPEG:
			FileMPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
		case FILE_AVI:
			FileMOV::get_parameters(parent_window, 
				asset, 
				format_window, 
				options,
				format->locked_compressor);
			break;
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			FileJPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
		case FILE_EXR:
		case FILE_EXR_LIST:
			FileEXR::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
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
		case FILE_OGG:
			FileOGG::get_parameters(parent_window,
				asset,
				format_window,
				options);
			break;
		default:
			break;
	}

	if(!format_window)
	{
		if(options & SUPPORTS_AUDIO)
			errorbox(_("This format doesn't support audio."));
		else
		if(options & SUPPORTS_VIDEO)
			errorbox(_("This format doesn't support video."));
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

void File::set_preload(int64_t size)
{
	this->playback_preload = size;
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
	this->preferences = preferences;
	this->asset->copy_from(asset, 1);
	file = 0;

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
		if(fread(test, 16, 1, stream) < 1)
		{
			fclose(stream);
			return FILE_NOT_FOUND;
		}
		if(strncmp(test, "TOC ", 4) == 0)
		{
			errormsg(_("Can't open TOC files directly"));
			return FILE_NOT_FOUND;
                }
		if(FileDV::check_sig(this->asset))
		{
// libdv
			fclose(stream);
			file = new FileDV(this->asset, this);
		}
		else 
		if(FilePNG::check_sig(this->asset))
		{
// PNG file
			fclose(stream);
			file = new FilePNG(this->asset, this);
		}
		else
		if(FileJPEG::check_sig(this->asset))
		{
// JPEG file
			fclose(stream);
			file = new FileJPEG(this->asset, this);
		}
		else
		if(FileEXR::check_sig(this->asset, test))
		{
// EXR file
			fclose(stream);
			file = new FileEXR(this->asset, this);
		}
		else
		if(FileYUV::check_sig(this->asset))
		{
// YUV file
			fclose(stream);
			file = new FileYUV(this->asset, this);
		}
		else
		if(FileTGA::check_sig(this->asset))
		{
// TGA file
			fclose(stream);
			file = new FileTGA(this->asset, this);
		}
		else
		if(FileTIFF::check_sig(this->asset))
		{
// TIFF file
			fclose(stream);
			file = new FileTIFF(this->asset, this);
		}
		else
		if(FileOGG::check_sig(this->asset))
		{
// OGG file
			fclose(stream);
			file = new FileOGG(this->asset, this);
		}
		else
		if(FileMPEG::check_sig(this->asset))
		{
// MPEG file
			fclose(stream);
			file = new FileMPEG(this->asset, this);
		}
		else
		if(FileSndFile::check_sig(this->asset))
		{
// libsndfile
			fclose(stream);
			file = new FileSndFile(this->asset, this);
		}
		else
		if(test[0] == '<' && test[1] == 'E' && test[2] == 'D' && test[3] == 'L' && test[4] == '>' ||
			test[0] == '<' && test[1] == 'H' && test[2] == 'T' && test[3] == 'A' && test[4] == 'L' && test[5] == '>' ||
			test[0] == '<' && test[1] == '?' && test[2] == 'x' && test[3] == 'm' && test[4] == 'l')
		{
// XML file
			fclose(stream);
			return FILE_IS_XML;
		}    // can't load project file
		else
		if(FileMOV::check_sig(this->asset))
		{
// MOV file
// should be last because quicktime lacks a magic number
			fclose(stream);
			file = new FileMOV(this->asset, this);
		}
		else
		{
// PCM file
			fclose(stream);
			return FILE_UNRECOGNIZED_CODEC;
		}   // need more info
		break;

// format already determined
	case FILE_AC3:
		file = new FileAC3(this->asset, this);
		break;

	case FILE_PCM:
	case FILE_WAV:
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

	case FILE_EXR:
	case FILE_EXR_LIST:
		file = new FileEXR(this->asset, this);
		break;

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

	case FILE_MOV:
		file = new FileMOV(this->asset, this);
		break;

	case FILE_MPEG:
	case FILE_AMPEG:
	case FILE_VMPEG:
		file = new FileMPEG(this->asset, this);
		break;

	case FILE_OGG:
		file = new FileOGG(this->asset, this);
		break;

	case FILE_AVI:
		file = new FileMOV(this->asset, this);
		break;

	case FILE_RAWDV:
		file = new FileDV(this->asset, this);
		break;

// try plugins
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
		if(use_cache &&frame_cache->get_frame(frame))
			return 0;
// Need temp
		if(frame->get_color_model() != BC_COMPRESSED &&
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
			temp_frame->copy_stacks(frame);
			temp_frame->copy_pts(frame);
			file->read_frame(temp_frame);
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
				temp_frame->get_w(), 
				temp_frame->get_h(),
				0, 
				0, 
				frame->get_w(), 
				frame->get_h(),
				temp_frame->get_color_model(), 
				frame->get_color_model(),
				0,
				temp_frame->get_w(),
				frame->get_w());
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

// Fill in queries about formats when adding formats here.

int File::strtoformat(char *format)
{
	return strtoformat(0, format);
}

int File::strtoformat(ArrayList<PluginServer*> *plugindb, char *format)
{
	if(!strcasecmp(format, _(AC3_NAME))) return FILE_AC3;
	else
	if(!strcasecmp(format, _(WAV_NAME))) return FILE_WAV;
	else
	if(!strcasecmp(format, _(PCM_NAME))) return FILE_PCM;
	else
	if(!strcasecmp(format, _(AU_NAME))) return FILE_AU;
	else
	if(!strcasecmp(format, _(AIFF_NAME))) return FILE_AIFF;
	else
	if(!strcasecmp(format, _(SND_NAME))) return FILE_SND;
	else
	if(!strcasecmp(format, _(PNG_NAME))) return FILE_PNG;
	else
	if(!strcasecmp(format, _(PNG_LIST_NAME))) return FILE_PNG_LIST;
	else
	if(!strcasecmp(format, _(TIFF_NAME))) return FILE_TIFF;
	else
	if(!strcasecmp(format, _(TIFF_LIST_NAME))) return FILE_TIFF_LIST;
	else
	if(!strcasecmp(format, _(JPEG_NAME))) return FILE_JPEG;
	else
	if(!strcasecmp(format, _(JPEG_LIST_NAME))) return FILE_JPEG_LIST;
	else
	if(!strcasecmp(format, _(EXR_NAME))) return FILE_EXR;
	else
	if(!strcasecmp(format, _(EXR_LIST_NAME))) return FILE_EXR_LIST;
	else
	if(!strcasecmp(format, _(YUV_NAME))) return FILE_YUV;
	else
	if(!strcasecmp(format, _(MPEG_NAME))) return FILE_MPEG;
	else
	if(!strcasecmp(format, _(AMPEG_NAME))) return FILE_AMPEG;
	else
	if(!strcasecmp(format, _(VMPEG_NAME))) return FILE_VMPEG;
	else
	if(!strcasecmp(format, _(TGA_NAME))) return FILE_TGA;
	else
	if(!strcasecmp(format, _(TGA_LIST_NAME))) return FILE_TGA_LIST;
	else
	if(!strcasecmp(format, _(MOV_NAME))) return FILE_MOV;
	else
	if(!strcasecmp(format, _(AVI_NAME))) return FILE_AVI;
	else
	if(!strcasecmp(format, _(OGG_NAME))) return FILE_OGG;
	else
	if(!strcasecmp(format, _(RAWDV_NAME))) return FILE_RAWDV;
	return 0;
}

int File::supports(int format)
{
	switch(format)
	{
	case FILE_JPEG:
	case FILE_JPEG_LIST:
	case FILE_PNG:
	case FILE_PNG_LIST:
	case FILE_TIFF:
	case FILE_TIFF_LIST:
	case FILE_EXR:
	case FILE_EXR_LIST:
	case FILE_TGA:
	case FILE_TGA_LIST:
		return SUPPORTS_VIDEO;

	case FILE_AIFF:
	case FILE_WAV:
	case FILE_PCM:
	case FILE_AU:
	case FILE_SND:
		return SUPPORTS_AUDIO;

	case FILE_AC3:
		return FileAC3::supports(format);

	case FILE_MPEG:
	case FILE_AMPEG:
	case FILE_VMPEG:
		return FileMPEG::supports(format);

	case FILE_OGG:
		return FileOGG::supports(format);

	case FILE_YUV:
		return FileYUV::supports(format);

	case FILE_RAWDV:
		return FileDV::supports(format);

	case FILE_AVI:
	case FILE_MOV:
		return FileMOV::supports(format);
	}
	return (SUPPORTS_AUDIO | SUPPORTS_VIDEO);
}

const char* File::formattostr(int format)
{
	return formattostr(0, format);
}

const char* File::formattostr(ArrayList<PluginServer*> *plugindb, int format)
{
	switch(format)
	{
	case FILE_AC3:
		return _(AC3_NAME);

	case FILE_WAV:
		return _(WAV_NAME);

	case FILE_PCM:
		return _(PCM_NAME);

	case FILE_AU:
		return _(AU_NAME);

	case FILE_AIFF:
		return _(AIFF_NAME);

	case FILE_SND:
		return _(SND_NAME);

	case FILE_PNG:
		return _(PNG_NAME);

	case FILE_PNG_LIST:
		return _(PNG_LIST_NAME);

	case FILE_JPEG:
		return _(JPEG_NAME);

	case FILE_JPEG_LIST:
		return _(JPEG_LIST_NAME);

	case FILE_EXR:
		return _(EXR_NAME);

	case FILE_EXR_LIST:
		return _(EXR_LIST_NAME);

	case FILE_YUV:
		return _(YUV_NAME);

	case FILE_MPEG:
		return _(MPEG_NAME);

	case FILE_AMPEG:
		return _(AMPEG_NAME);

	case FILE_VMPEG:
		return _(VMPEG_NAME);

	case FILE_TGA:
		return _(TGA_NAME);

	case FILE_TGA_LIST:
		return _(TGA_LIST_NAME);

	case FILE_TIFF:
		return _(TIFF_NAME);

	case FILE_TIFF_LIST:
		return _(TIFF_LIST_NAME);

	case FILE_MOV:
		return _(MOV_NAME);

	case FILE_AVI:
		return _(AVI_NAME);

	case FILE_OGG:
		return _(OGG_NAME);

	case FILE_RAWDV:
		return _(RAWDV_NAME);

	default:
		return _("Unknown");
	}
}

int File::strtobits(char *bits)
{
	if(!strcasecmp(bits, _(NAME_8BIT))) return BITSLINEAR8;
	if(!strcasecmp(bits, _(NAME_16BIT))) return BITSLINEAR16;
	if(!strcasecmp(bits, _(NAME_24BIT))) return BITSLINEAR24;
	if(!strcasecmp(bits, _(NAME_32BIT))) return BITSLINEAR32;
	if(!strcasecmp(bits, _(NAME_ULAW))) return BITSULAW;
	if(!strcasecmp(bits, _(NAME_ADPCM))) return BITS_ADPCM;
	if(!strcasecmp(bits, _(NAME_FLOAT))) return BITSFLOAT;
	if(!strcasecmp(bits, _(NAME_IMA4))) return BITSIMA4;
	return BITSLINEAR16;
}

const char* File::bitstostr(int bits)
{
	switch(bits)
	{
	case BITSLINEAR8:
		return (NAME_8BIT);

	case BITSLINEAR16:
		return (NAME_16BIT);

	case BITSLINEAR24:
		return (NAME_24BIT);

	case BITSLINEAR32:
		return (NAME_32BIT);

	case BITSULAW:
		return (NAME_ULAW);

	case BITS_ADPCM:
		return (NAME_ADPCM);

	case BITSFLOAT:
		return (NAME_FLOAT);

	case BITSIMA4:
		return (NAME_IMA4);
	}
	return "Unknown";
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

	case FILE_MOV:
		return FileMOV::get_best_colormodel(asset, driver);

	case FILE_AVI:
		return FileMOV::get_best_colormodel(asset, driver);

	case FILE_MPEG:
		return FileMPEG::get_best_colormodel(asset, driver);

	case FILE_JPEG:
	case FILE_JPEG_LIST:
		return FileJPEG::get_best_colormodel(asset, driver);

	case FILE_EXR:
	case FILE_EXR_LIST:
		return FileEXR::get_best_colormodel(asset, driver);

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

int64_t File::get_memory_usage() 
{
	int64_t result = 0;
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

int File::supports_video(ArrayList<PluginServer*> *plugindb, char *format)
{
	int i, format_i = strtoformat(plugindb, format);

	return supports_video(format_i);
}

int File::supports_audio(ArrayList<PluginServer*> *plugindb, char *format)
{
	int i, format_i = strtoformat(plugindb, format);

	return supports_audio(format_i);
}

int File::supports_video(int format)
{
	switch(format)
	{
	case FILE_OGG:
	case FILE_MOV:
	case FILE_JPEG:
	case FILE_JPEG_LIST:
	case FILE_EXR:
	case FILE_EXR_LIST:
	case FILE_YUV:
	case FILE_PNG:
	case FILE_PNG_LIST:
	case FILE_TGA:
	case FILE_TGA_LIST:
	case FILE_TIFF:
	case FILE_TIFF_LIST:
	case FILE_VMPEG:
	case FILE_AVI:
	case FILE_RAWDV:
			return 1;

	default:
		return 0;
	}
}

int File::supports_audio(int format)
{
	switch(format)
	{
	case FILE_AC3:
	case FILE_PCM:
	case FILE_WAV:
	case FILE_MOV:
	case FILE_OGG:
	case FILE_AMPEG:
	case FILE_AU:
	case FILE_AIFF:
	case FILE_SND:
	case FILE_AVI:
	case FILE_RAWDV:
		return 1;

	default:
		return 0;
	}
}

const char* File::get_tag(int format)
{
	switch(format)
	{
	case FILE_AC3:
		return "ac3";
	case FILE_AIFF:
		return "aif";
	case FILE_AMPEG:
		return "mp3";
	case FILE_AU:
		return "au";
	case FILE_AVI:
		return "avi";
	case FILE_RAWDV:
		return "dv";
	case FILE_EXR:
		return "exr";
	case FILE_JPEG:
		return "jpg";
	case FILE_MOV:
		return "mov";
	case FILE_OGG:
		return "ogg";
	case FILE_PCM:
		return "pcm";
	case FILE_PNG:
		return "png";
	case FILE_TGA:
		return "tga";
	case FILE_TIFF:
		return "tif";
	case FILE_VMPEG:
		return "m2v";
	case FILE_WAV:
		return "wav";
	case FILE_YUV:
		return "m2v";
	case FILE_EXR_LIST:
	case FILE_JPEG_LIST:
	case FILE_TGA_LIST:
	case FILE_TIFF_LIST:
	case FILE_PNG_LIST:
		return "list";
	}
	return 0;
}

PackagingEngine *File::new_packaging_engine(Asset *asset)
{
	return (PackagingEngine*) new PackagingEngineDefault();
}
