
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
#include "bcsignals.h"
#include "bctitle.h"
#include "clip.h"
#include "file.h"
#include "filesndfile.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "selection.h"
#include "theme.h"


FileSndFile::FileSndFile(Asset *asset, File *file)
 : FileBase(asset, file)
{
	temp_double = 0;
	temp_allocated = 0;
	fd_config.format = 0;
	fd = 0;
	bufpos = -1;
	buf_fill = 0;
	buf_end = 0;
}

FileSndFile::~FileSndFile()
{
	if(temp_double) delete [] temp_double;
}

void FileSndFile::asset_to_format()
{
	switch(asset->format)
	{
	case FILE_PCM:
		fd_config.format = SF_FORMAT_RAW;
		break;
	case FILE_WAV:
		fd_config.format = SF_FORMAT_WAV;
		break;
	case FILE_AU:
		fd_config.format = SF_FORMAT_AU;
		break;
	case FILE_AIFF:
		fd_config.format = SF_FORMAT_AIFF;
		break;
	}

// Not all of these are allowed in all sound formats.
// Raw can't be float.
	switch(asset->bits)
	{
	case 8:
		if(asset->format == FILE_WAV)
			fd_config.format |= SF_FORMAT_PCM_U8;
		else
		if(asset->signed_)
			fd_config.format |= SF_FORMAT_PCM_S8;
		else
			fd_config.format |= SF_FORMAT_PCM_U8;
		break;

	case 16:
		fd_config.format |= SF_FORMAT_PCM_16;
		if(asset->byte_order || asset->format == FILE_WAV)
			fd_config.format |= SF_ENDIAN_LITTLE;
		else
			fd_config.format |= SF_ENDIAN_BIG;
		break;

	case 24:
		fd_config.format |= SF_FORMAT_PCM_24;

		if(asset->byte_order || asset->format == FILE_WAV)
			fd_config.format |= SF_ENDIAN_LITTLE;
		else
			fd_config.format |= SF_ENDIAN_BIG;
		break;

	case 32:
		fd_config.format |= SF_FORMAT_PCM_32;

		if(asset->byte_order || asset->format == FILE_WAV)
			fd_config.format |= SF_ENDIAN_LITTLE;
		else
			fd_config.format |= SF_ENDIAN_BIG;
		break;

	case SBITS_ULAW: 
		fd_config.format |= SF_FORMAT_ULAW; 
		break;

	case SBITS_FLOAT:
		fd_config.format |= SF_FORMAT_FLOAT; 
		break;

	case SBITS_ADPCM:
		if(fd_config.format == FILE_WAV)
			fd_config.format |= SF_FORMAT_MS_ADPCM;
		else
			fd_config.format |= SF_FORMAT_IMA_ADPCM; 
		fd_config.format |= SF_FORMAT_PCM_16;
		break;
	}

	fd_config.seekable = 1;
	fd_config.samplerate = asset->sample_rate;
	fd_config.channels  = asset->channels;
}

void FileSndFile::format_to_asset()
{
// User supplies values if PCM
	if(asset->format == 0)
	{
		asset->byte_order = 0;
		asset->signed_ = 1;
		switch(fd_config.format & SF_FORMAT_TYPEMASK)
		{
		case SF_FORMAT_WAV:
			asset->format = FILE_WAV;  
			asset->byte_order = 1;
			asset->header = 44;
			break;
		case SF_FORMAT_AIFF:
			asset->format = FILE_AIFF;
			break;
		case SF_FORMAT_AU:
			asset->format = FILE_AU;
			break;
		case SF_FORMAT_RAW:
			asset->format = FILE_PCM;
			break;
		case SF_FORMAT_PAF:
			asset->format = FILE_SND;
			break;
		case SF_FORMAT_SVX:
			asset->format = FILE_SND;
			break;
		case SF_FORMAT_NIST:
			asset->format = FILE_SND;
			break;
		}

		switch(fd_config.format & SF_FORMAT_SUBMASK)
		{
		case SF_FORMAT_FLOAT: 
			asset->bits = SBITS_FLOAT;
			break;
		case SF_FORMAT_ULAW: 
			asset->bits = SBITS_ULAW;
			break;
		case SF_FORMAT_IMA_ADPCM:
		case SF_FORMAT_MS_ADPCM:
			asset->bits = SBITS_ADPCM;
			break;
		case SF_FORMAT_PCM_16:
			asset->signed_ = 1;
			asset->bits = 16;
			break;
		case SF_FORMAT_PCM_24:
			asset->signed_ = 1;
			asset->bits = 24;
			break;
		case SF_FORMAT_PCM_32:
			asset->signed_ = 1;
			asset->bits = 32;
			break;
		case SF_FORMAT_PCM_S8:
			asset->signed_ = 1;
			asset->bits = 8;
			break;
		case SF_FORMAT_PCM_U8:
			asset->signed_ = 0;
			asset->bits = 8;
			break;
		}

		switch(fd_config.format & SF_FORMAT_ENDMASK)
		{
		case SF_ENDIAN_LITTLE:
			asset->byte_order = 1;
			break;
		case SF_ENDIAN_BIG:
			asset->byte_order = 0;
			break;
		}

		asset->channels = fd_config.channels;
	}

	asset->audio_data = 1;
	asset->audio_length = fd_config.frames;
	if(!asset->sample_rate)
		asset->sample_rate = fd_config.samplerate;
	if(asset->audio_length && asset->sample_rate)
		asset->audio_duration = (ptstime)asset->audio_length / asset->sample_rate;
	asset->init_streams();
}

int FileSndFile::open_file(int open_mode)
{
	int result = 0;

	if(open_mode & FILE_OPEN_READ)
	{
		if(asset->format == FILE_PCM)
		{
			asset_to_format();
			fd = sf_open(asset->path, SFM_READ, &fd_config);
// Already given by user
			if(fd) format_to_asset();
		}
		else
		{
			fd = sf_open(asset->path, SFM_READ, &fd_config);
// Doesn't calculate the length
			if(fd) format_to_asset();
		}
	}
	else
	if(open_mode & FILE_OPEN_WRITE)
	{
		asset_to_format();
		fd = sf_open(asset->path, SFM_WRITE, &fd_config);
	}

	if(!fd) 
	{
		result = 1;
		errormsg("%s", sf_strerror(0));
	}

	return result;
}

void FileSndFile::close_file()
{
	if(fd) sf_close(fd);
	fd = 0;
	fd_config.format = 0;
}

int FileSndFile::read_aframe(AFrame *aframe)
{
	int result = 0;
	sf_count_t rqpos = aframe->position;
	int len = aframe->source_length;

// Get temp buffer for interleaved channels
	if(temp_allocated && temp_allocated < len)
	{
		delete [] temp_double;
		temp_double = 0;
		temp_allocated = 0;
	}

	if(!temp_allocated)
	{
		temp_allocated = len;
		temp_double = new double[len * asset->channels];
		bufpos = -1;
	}
	if(rqpos != bufpos || len != buf_fill)
	{
		if(rqpos != buf_end)
		{
			if(sf_seek(fd, rqpos, SEEK_SET) < 0)
				errormsg("sf_seek() to sample %" PRId64 " failed, reason: %s",
					rqpos, sf_strerror(fd));
			bufpos = rqpos;
			buf_fill = 0;
		}
		else
			bufpos = buf_end;

		buf_fill = sf_readf_double(fd, temp_double, len);
		buf_end = bufpos + buf_fill;
	}
// Extract single channel
	double *buffer = &aframe->buffer[aframe->length];

	for(int i = 0, j = aframe->channel;
		i < buf_fill;
		i++, j += asset->channels)
	{
		buffer[i] = temp_double[j];
	}
	aframe->set_filled_length();
	return result;
}

int FileSndFile::write_aframes(AFrame **frames)
{
	int result = 0;
	int len = frames[0]->length;

// Get temp buffer for interleaved channels
	if(temp_allocated && temp_allocated < len)
	{
		temp_allocated = 0;
		delete [] temp_double;
		temp_double = 0;
	}

	if(!temp_allocated)
	{
		temp_allocated = len;
		temp_double = new double[len * asset->channels];
	}

// Interleave channels
	for(int i = 0; i < asset->channels; i++)
	{
		for(int j = 0; j < len; j++)
		{
			double sample = frames[i]->buffer[j];
// Libsndfile does not limit values
			if(asset->bits != SBITS_FLOAT) CLAMP(sample, -1.0, (32767.0 / 32768.0));
			temp_double[j * asset->channels + i] = sample;
		}
	}

	result = !sf_writef_double(fd, temp_double, len);

	return result;
}

void FileSndFile::get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int options)
{
	if(options & SUPPORTS_AUDIO)
	{
		if(asset->format == FILE_AU)
		{
			FBConfig *window = new FBConfig(parent_window, options);
			format_window = window;
		}
		else
		{
			SndFileConfig *window = new SndFileConfig(parent_window, asset);
			format_window = window;
		}
		format_window->run_window();
		delete format_window;
	}
}

SndFileConfig::SndFileConfig(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(MWindow::create_title(N_("Audio Compression")),
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	250,
	250)
{
	int x = 10, y = 10;

	this->asset = asset;
	set_icon(theme_global->get_image("mwindow_icon"));

	switch(asset->format)
	{
	case FILE_WAV:
	case FILE_PCM:
	case FILE_AIFF:
		add_tool(new BC_Title(x, y, _("Compression:")));
		y += 25;

		int bits = SBITS_LINEAR8 | SBITS_LINEAR16 | SBITS_LINEAR24;
		SampleBitsSelection *bitspopup;

		if(asset->format == FILE_WAV)
			bits |= SBITS_ADPCM | SBITS_FLOAT;
		add_subwindow(bitspopup = new SampleBitsSelection(x, y, this,
			&asset->bits, bits));
		bitspopup->update_size(asset->bits);
		y += 40;
		break;
	}

	x = 10;
	if(asset->format != FILE_AU)
		add_subwindow(new BC_CheckBox(x, y, &asset->dither, _("Dither")));
	y += 30;
	if(asset->format == FILE_PCM)
	{
		add_subwindow(new BC_CheckBox(x, y, &asset->signed_, _("Signed")));
		y += 35;
		add_subwindow(new BC_Title(x, y, _("Byte order:")));
		add_subwindow(hilo = new SndFileHILO(this, x + 100, y));
		add_subwindow(lohi = new SndFileLOHI(this, x + 170, y));
	}
	add_subwindow(new BC_OKButton(this));
}


SndFileHILO::SndFileHILO(SndFileConfig *gui, int x, int y)
 : BC_Radial(x, y, gui->asset->byte_order == 0, _("Hi Lo"))
{
	this->gui = gui;
}
int SndFileHILO::handle_event()
{
	gui->asset->byte_order = 0;
	gui->lohi->update(0);
	return 1;
}


SndFileLOHI::SndFileLOHI(SndFileConfig *gui, int x, int y)
 : BC_Radial(x, y, gui->asset->byte_order == 1, _("Lo Hi"))
{
	this->gui = gui;
}

int SndFileLOHI::handle_event()
{
	gui->asset->byte_order = 1;
	gui->hilo->update(0);
	return 1;
}
