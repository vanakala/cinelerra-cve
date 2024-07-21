// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "aframe.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "cache.inc"
#include "condition.h"
#include "edit.h"
#include "file.h"
#include "fileavlibs.h"
#include "filebase.h"
#include "filexml.h"
#include "filejpeg.h"
#include "filepng.h"
#include "filetga.h"
#include "filetiff.h"
#include "formattools.h"
#include "framecache.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "pluginserver.h"
#include "tmpframecache.h"
#include "vframe.h"

File::File()
{
	cpus = 1;
	asset = 0;
	format_completion = new Mutex("File::format_completion");
	write_lock = new Condition(1, "File::write_lock");
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
	close_file();
	delete format_completion;
	delete write_lock;
}

void File::reset_parameters()
{
	file = 0;
	writing = 0;
	getting_options = 0;
	format_window = 0;
	temp_frame = 0;
	last_frame = 0;
}

void File::raise_window()
{
	if(getting_options && format_window)
		format_window->raise_window();
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
		case FILE_AC3:
		case FILE_AVI:
		case FILE_MOV:
		case FILE_OGG:
		case FILE_WAV:
		case FILE_MP3:
		case FILE_AIFF:
		case FILE_AU:
		case FILE_PCM:
		case FILE_FLAC:
		case FILE_MPEG:
		case FILE_MPEGTS:
		case FILE_RAWDV:
		case FILE_YUV:
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
		case FILE_EXR:
		case FILE_AAC:
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

void File::set_processors(int cpus)   // Set the number of cpus for certain codecs
{
	this->cpus = cpus;
}

int File::is_imagelist(int format)
{
	switch(format)
	{
	case FILE_JPEG_LIST:
	case FILE_TGA_LIST:
	case FILE_TIFF_LIST:
	case FILE_PNG_LIST:
		return 1;
	}
	return 0;
}

int File::probe_file(Asset *asset)
{
	FILE *stream;
	int probe_result = 0;
	int rs;

	if(!asset->probed)
	{
		FileAVlibs *probe = new FileAVlibs(asset, this);
		probe_result = probe->probe_input(asset);
		delete probe;

		if(probe_result < 0)
			return FILE_NOT_FOUND;

		asset->probed = 1;
// Probe input fills decoder parameters what
// are not used with some formats
		switch(asset->format)
		{
		case FILE_JPEG:
		case FILE_PNG:
		case FILE_TGA:
		case FILE_SND:
			asset->delete_decoder_parameters();
			break;

		case FILE_TIFF:
			asset->delete_decoder_parameters();
			if(!FileTIFF::check_sig(asset))
				return FILE_UNRECOGNIZED_CODEC;
			break;

		case FILE_UNKNOWN:
			if(!(stream = fopen(asset->path, "rb")))
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
			// Fall through
		case FILE_JPEG_LIST:
		case FILE_PNG_LIST:
		case FILE_TGA_LIST:
		case FILE_TIFF_LIST:
			if(FileList::probe_list(asset))
			{
				if(strncmp(test, "<EDL", 4) == 0 ||
						strncmp(test, "<?xml", 5) == 0)
					return FILE_IS_XML;
				else
					return FILE_UNRECOGNIZED_CODEC;
			}
			break;
		}
	}
	return FILE_OK;
}

int File::open_file(Asset *asset, int open_method, int stream, const char *filepath)
{
	int probe_result, rs;
	int rd, wr;

	rd = open_method & FILE_OPEN_READ;
	wr = open_method & FILE_OPEN_WRITE;

	this->asset = asset;
	asset_stream = stream;
	file = 0;

	switch(asset->format)
	{
	case FILE_PNG:
	case FILE_PNG_LIST:
		file = new FilePNG(asset, this);
		break;

	case FILE_JPEG:
	case FILE_JPEG_LIST:
		file = new FileJPEG(asset, this);
		break;

	case FILE_TGA_LIST:
	case FILE_TGA:
		file = new FileTGA(asset, this);
		break;

	case FILE_TIFF:
	case FILE_TIFF_LIST:
		file = new FileTIFF(asset, this);
		break;

	case FILE_SVG:
	case FILE_AC3:
	case FILE_AVI:
	case FILE_MOV:
	case FILE_OGG:
	case FILE_MP3:
	case FILE_WAV:
	case FILE_AIFF:
	case FILE_AU:
	case FILE_PCM:
	case FILE_FLAC:
	case FILE_MPEGTS:
	case FILE_MPEG:
	case FILE_RAWDV:
	case FILE_YUV:
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
	case FILE_AAC:
	case FILE_EXR:
		file = new FileAVlibs(asset, this);
		break;

	default:
		errormsg("No suitable codec for format '%s'",
			ContainerSelection::container_to_text(asset->format));
		return FILE_UNRECOGNIZED_CODEC;
	}

	if(file->open_file(open_method, stream, filepath))
	{
		delete file;
		file = 0;
	}

	if(file && wr)
		writing = 1;

	if(file)
		return FILE_OK;
	else
		return FILE_NOT_FOUND;
}

void File::close_file()
{
	if(file)
	{
		file->close_file();
		delete file;
	}
	delete temp_frame;
	BC_Resources::tmpframes.release_frame(last_frame);
	reset_parameters();
}

samplenum File::get_audio_length()
{
	return asset->stream_samples(asset_stream);
}

// No resampling here.
int File::write_aframes(AFrame **frames)
{
	int result;

	if(file)
	{
		write_lock->lock("File::write_aframes");
		result = file->write_aframes(frames);
		write_lock->unlock();
		return result;
	}
	return 1;
}

int File::write_frames(VFrame **frames, int len)
{
	int result;

	write_lock->lock("File::write_frames");
	result = file->write_frames(frames, len);
	write_lock->unlock();

	return result;
}

int File::get_samples(AFrame *aframe)
{
	if(!file)
		return 0;

	if(!aframe->get_samplerate() || aframe->get_buffer_length() <= 0 ||
			aframe->get_source_duration() <= 0)
		return 0;

	aframe->position = round(aframe->get_source_pts() * aframe->get_samplerate());
	if(aframe->get_length() + aframe->get_source_length() > aframe->get_buffer_length())
		aframe->set_source_length(aframe->get_buffer_length() - aframe->get_length());
	if(aframe->get_source_length() <= 0)
		return 0;
// Never try to read more samples than exist in the file
	if(aframe->get_source_pts() + aframe->get_source_duration() > asset->stream_duration(asset_stream))
		aframe->set_source_duration(asset->stream_duration(asset_stream) - aframe->get_source_pts());

	return file->read_aframe(aframe);
}

int File::get_frame(VFrame *frame)
{
	ptstime rqpts, srcrqpts;
	int result;

	if(file)
	{
		int supported_colormodel = colormodel_supported(frame->get_color_model());
		srcrqpts = frame->get_source_pts();
		rqpts = frame->get_pts();

		if(asset->single_image)
			frame->set_source_pts(0);
// Test cache
		if(last_frame && last_frame->pts_in_frame_source(srcrqpts, FRAME_ACCURACY) &&
			fabs(last_frame->next_source_pts() - srcrqpts) < FRAME_ACCURACY)
		{
			if(frame->equivalent(last_frame))
			{
				frame->copy_from(last_frame);
				frame->copy_pts(last_frame);
				frame->set_pts(rqpts);
				adjust_times(frame, rqpts, srcrqpts);
				return FILE_OK;
			}
			BC_Resources::tmpframes.release_frame(last_frame);
			last_frame = 0;
		}
// Need temp
		if(frame->get_color_model() != BC_COMPRESSED &&
			!file->converts_frame() &&
			(supported_colormodel != frame->get_color_model() ||
			frame->get_w() != asset->streams[asset_stream].width ||
			frame->get_h() != asset->streams[asset_stream].height))
		{
			if(temp_frame)
			{
				if(!temp_frame->params_match(
					asset->streams[asset_stream].width,
					asset->streams[asset_stream].height,
					supported_colormodel))
				{
					delete temp_frame;
					temp_frame = 0;
				}
			}

			if(!temp_frame)
			{
				temp_frame = new VFrame(0,
					asset->streams[asset_stream].width,
					asset->streams[asset_stream].height,
					supported_colormodel);
			}
			temp_frame->copy_pts(frame);
			result = file->read_frame(temp_frame);
			frame->transfer_from(temp_frame);
			frame->copy_pts(temp_frame);
		}
		else
			result = file->read_frame(frame);

		if(asset->single_image)
		{
			frame->set_source_pts(0);
			frame->set_duration(asset->stream_duration(asset_stream));
			frame->set_frame_number(0);
		}

		if(last_frame && !last_frame->equivalent(frame))
		{
			BC_Resources::tmpframes.release_frame(last_frame);
			last_frame = 0;
		}
		if(!last_frame)
		{
			last_frame = BC_Resources::tmpframes.get_tmpframe(
				frame->get_w(), frame->get_h(),
				frame->get_color_model(), "File::get_frame");
		}
		last_frame->copy_from(frame);
		last_frame->copy_pts(frame);
		adjust_times(frame, rqpts, srcrqpts);
		return result;
	}
	else
		return FILE_NOT_FOUND;
}

void File::adjust_times(VFrame *frame, ptstime pts, ptstime src_pts)
{
	frame->set_pts(pts);
	if(frame->get_source_pts() < src_pts)
		frame->set_duration(frame->get_duration() -
			(src_pts - frame->get_source_pts()));
}

int File::supports(int format)
{
	switch(format)
	{
	case FILE_JPEG:
	case FILE_PNG:
	case FILE_TIFF:
	case FILE_TGA:
		return SUPPORTS_VIDEO | SUPPORTS_STILL;

	case FILE_JPEG_LIST:
	case FILE_PNG_LIST:
	case FILE_TIFF_LIST:
	case FILE_TGA_LIST:
		return SUPPORTS_VIDEO;

	case FILE_SND:
		return SUPPORTS_AUDIO;

	case FILE_YUV:
	case FILE_AC3:
	case FILE_AVI:
	case FILE_MOV:
	case FILE_OGG:
	case FILE_MP3:
	case FILE_WAV:
	case FILE_AIFF:
	case FILE_AU:
	case FILE_PCM:
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
	case FILE_AAC:
	case FILE_EXR:
		return FileAVlibs::supports(format, 0);
	}
	return 0;
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
	if(last_frame) result += last_frame->get_data_size();
	if(result < MIN_CACHEITEM_SIZE) result = MIN_CACHEITEM_SIZE;
	return result;
}
