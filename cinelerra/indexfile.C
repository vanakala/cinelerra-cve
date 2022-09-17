// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "clip.h"
#include "condition.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "file.h"
#include "indexfile.h"
#include "indexthread.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "preferences.h"
#include "resourcepixmap.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "vframe.h"

#include <unistd.h>

// Use native sampling rates for files so the same index can be used in
// multiple projects.

IndexFile::IndexFile()
{
	file = 0;
	interrupt_flag = 0;
	asset = 0;
	index_filename = 0;
	status = INDEX_NOTTESTED;
	start = index_end = old_index_end = 0;
	memset(offsets, 0, sizeof(offsets));
	memset(sizes, 0, sizeof(sizes));
	index_buffer = 0;
	buffer_allocated = 0;
}

IndexFile::~IndexFile()
{
	delete [] index_filename;
	delete [] index_buffer;
}

int IndexFile::open_index(Asset *asset, int stream)
{
	int result = 0;

	if(asset == this->asset && stream == this->stream && file &&
			status == INDEX_READY)
		return 1;

	close_index();

	this->asset = asset;
	this->stream = stream;

	if(status == INDEX_BUILDING)
		return 2;
	else if(!(result = open_file()))
		status = INDEX_READY;
	else
	{
		status = INDEX_NOTTESTED;
		return 1;
	}

	return result;
}

void IndexFile::delete_index()
{
	if(index_filename)
		remove(index_filename);
	delete [] index_filename;
	index_filename = 0;
}

int IndexFile::open_file()
{
	struct stat file_stat, idx_stat;
	int result = 0;
	char source_filename[BCTEXTLEN];

	if(!index_filename)
	{
		index_filename = new char[BCTEXTLEN];

		get_index_filename(source_filename,
			preferences_global->index_directory,
			index_filename,
			asset->path, stream);
	}

	if(file = fopen(index_filename, "rb"))
	{
// Index file already exists.
		memset(&file_stat, 0, sizeof(struct stat));
		memset(&idx_stat, 0, sizeof(struct stat));
		stat(index_filename, &idx_stat);
		stat(asset->path, &file_stat);

		if(read_info() || idx_stat.st_mtime < file_stat.st_mtime)
		{
// index older than source
			result = 2;
			fclose(file);
			file = 0;
		}
		else
		if(file_stat.st_size != asset->index_bytes)
		{
// source file is a different size than index source file
			result = 2;
			fclose(file);
			file = 0;
		}
		else
		{
			file_length = idx_stat.st_size;
			result = 0;
		}
	}
	else
	{
// doesn't exist
		result = 1;
	}

	return result;
}

int IndexFile::open_source(File *source)
{
	if(!source->open_file(asset, FILE_OPEN_READ | FILE_OPEN_AUDIO, stream))
	{
		FileSystem fs;
		asset->index_bytes = fs.get_size(asset->path);
		return 0;
	}
	return 1;
}

int IndexFile::get_required_scale(File *source)
{
	int result;

	samplenum length_source =
		source->asset->stream_samples(stream);

	if(length_source > 0)
	{
		int peak_count =
			(preferences_global->index_size * KILOBYTE) /
			(2 * sizeof(float) * asset->streams[stream].channels);

		for(result = 1;
			length_source / result > peak_count;
			result *= 2);
	} else
		return 0;

	return result;
}

void IndexFile::get_index_filename(char *source_filename, 
	const char *index_directory,
	char *index_filename, 
	const char *input_filename, int stream,
	const char *extension)
{
// Replace slashes and dots
	int i, j;
	int len = strlen(input_filename);
	for(i = 0, j = 0; i < len; i++)
	{
		if(input_filename[i] != '/' &&
				input_filename[i] != '.')
			source_filename[j++] = input_filename[i];
		else
		{
			if(i > 0)
				source_filename[j++] = '_';
		}
	}
	if(stream >= 0)
		j += sprintf(&source_filename[j], ":%02d", stream);
	source_filename[j] = 0;
	FileSystem fs;
	fs.join_names(index_filename, index_directory, source_filename);
	if(!extension)
		extension = INDEXFILE_EXTENSION;
	strcat(index_filename, extension);
}

void IndexFile::interrupt_index()
{
	interrupt_flag = 1;
}

void IndexFile::allocate_buffer(int index_size)
{
	if(index_buffer && buffer_allocated > index_size)
		return;

	delete [] index_buffer;
	index_buffer = new float[index_size];

	memset(index_buffer, 0, index_size * sizeof(float));
	memset(offsets, 0, sizeof(offsets));
	memset(sizes, 0, sizeof(sizes));
}

void IndexFile::release_buffer()
{
	delete [] index_buffer;
	index_buffer = 0;
}

int IndexFile::create_index(Asset *asset, int stream, MainProgressBar *progress)
{
	int result = 0;
	File source;
	char source_filename[BCTEXTLEN];

	this->asset = asset;
	this->stream = stream;
	interrupt_flag = 0;

	if(open_source(&source))
		return 1;

	if(!index_filename)
	{
		index_filename = new char[BCTEXTLEN];

		get_index_filename(source_filename,
			preferences_global->index_directory,
			index_filename,
			asset->path, stream);
	}

// Build index from scratch
	if(!(zoom = get_required_scale(&source)))
		return 1;
// total length of input file
	samplenum length_source = source.asset->stream_samples(stream);

// get amount to read at a time in floats
	int buffersize = 65536;

	progress->update_title(_("Creating %s."), index_filename);
	progress->update_length(length_source);
	redraw_timer.update();

// thread out index thread
	IndexThread *index_thread = new IndexThread(this,
		asset, stream, index_filename,
		buffersize, length_source);
	index_thread->start_build();

// current sample in source file
	samplenum position = 0;
	int fragment_size = buffersize;
	int current_buffer = 0;

// pass through file once
	while(position < length_source && !result)
	{
		if(length_source - position < fragment_size && fragment_size == buffersize)
			fragment_size = length_source - position;

		index_thread->input_lock[current_buffer]->lock("IndexFile::create_index 1");
		int cancelled = progress->update(position);

		if(cancelled || index_thread->interrupt_flag ||
			interrupt_flag)
		{
			result = 3;
		}

		int nb_channels =  asset->streams[stream].channels;

		for(int channel = 0; !result && channel < nb_channels; channel++)
		{
			index_thread->frames_in[current_buffer][channel]->set_fill_request(position, fragment_size);
// Read from source file
			if(source.get_samples(index_thread->frames_in[current_buffer][channel]))
				result = 1;
		}

// Release buffer to thread
		if(!result)
		{
			index_thread->output_lock[current_buffer]->unlock();
			current_buffer++;
			if(current_buffer >= TOTAL_BUFFERS)
				current_buffer = 0;
			position += fragment_size;
		}
		else
			index_thread->input_lock[current_buffer]->unlock();
	}

// end thread cleanly
	index_thread->input_lock[current_buffer]->lock("IndexFile::create_index 2");
	index_thread->last_buffer[current_buffer] = 1;
	index_thread->output_lock[current_buffer]->unlock();
	index_thread->stop_build();

	delete index_thread;

	source.close_file();
	open_index(asset, stream);
	close_index();
	return 0;
}

void IndexFile::redraw_edits(int force)
{
	int64_t difference = redraw_timer.get_scaled_difference(1000);

	if(difference > 250 || force)
	{
		redraw_timer.update();
		mwindow_global->draw_indexes(asset);
		old_index_end = index_end;
	}
}

int IndexFile::draw_index(ResourcePixmap *pixmap, Edit *edit, int x, int w)
{
// test channel number
	if(edit->channel >= asset->streams[stream].channels)
		return 1;

// check against index_end when being built
	if(zoom == 0)
	{
		errorbox(_("IndexFile::draw_index: index of stream %d has 0 zoom"),
			stream);
		return 0;
	}

// calculate a virtual x where the edit_x should be in floating point
	double virtual_edit_x = round((edit->get_pts() - master_edl->local_session->view_start_pts) /
			master_edl->local_session->zoom_time);
// samples in segment to draw relative to asset
	double asset_over_session = (double)asset->streams[stream].sample_rate /
		edlsession->sample_rate;
	samplenum startsource = round(((pixmap->pixmap_x - virtual_edit_x + x) *
		(master_edl->local_session->zoom_time * edlsession->sample_rate) +
		edit->track->to_units(edit->get_source_pts())) *
		asset_over_session);
// just in case we get a numerical error 
	if(startsource < 0)
		startsource = 0;

	int length = round(w * master_edl->local_session->zoom_time *
		edlsession->sample_rate * asset_over_session);

	if(status == INDEX_BUILDING)
	{
		if(startsource + length > index_end)
			length = index_end - startsource;
	}

// length of index to read in samples * 2
	int lengthindex = length / zoom * 2;
// start of data in samples
	int startindex = startsource / zoom * 2;
// Clamp length of index to read by available data
	if(startindex + lengthindex > sizes[edit->channel])
		lengthindex = sizes[edit->channel] - startindex;

	if(lengthindex <= 0)
		return 0;

// Actual length read from file in bytes
	int length_read;
// Start and length of fragment to read from file in bytes.
	int startfile, lengthfile;
	float *buffer = 0;
	int buffer_shared = 0;
	int i;
	int center_pixel = master_edl->local_session->zoom_track / 2;

	if(edlsession->show_titles)
		center_pixel += theme_global->get_image("title_bg_data")->get_h();

	int miny = center_pixel - master_edl->local_session->zoom_track / 2;
	int maxy = center_pixel + master_edl->local_session->zoom_track / 2;

	int x1 = 0, y1, y2;

	double index_frames_per_pixel = master_edl->local_session->zoom_time *
		edlsession->sample_rate /
		zoom * asset_over_session;
// get channel offset
	startindex += offsets[edit->channel];

	if(status == INDEX_BUILDING)
	{
// index is in RAM, being built
		buffer = &index_buffer[startindex];
		buffer_shared = 1;
	}
	else
	{
// index is stored in a file
		buffer = new float[lengthindex + 1];
		buffer_shared = 0;
		startfile = start + startindex * sizeof(float);
		lengthfile = lengthindex * sizeof(float);
		length_read = 0;

		if(startfile < file_length)
		{
			fseek(file, startfile, SEEK_SET);

			length_read = lengthfile;
			if(startfile + length_read > file_length)
				length_read = file_length - startfile;

			if(fread(buffer, length_read, 1, file) < 1)
				errormsg("Failed to read index");
		}

		if(length_read < lengthfile)
			for(i = length_read / sizeof(float); 
					i < lengthindex; i++)
				buffer[i] = 0;
	}

	double current_frame = 0;
	double highsample = buffer[0];
	double lowsample = buffer[1];
	int prev_y1 = center_pixel;
	int prev_y2 = center_pixel;
	int first_frame = 1;

	for(int bufferposition = 0; bufferposition < lengthindex;
			bufferposition += 2)
	{
		if(current_frame >= index_frames_per_pixel)
		{
			int next_y1 = round(center_pixel - highsample * master_edl->local_session->zoom_y / 2);
			int next_y2 = round(center_pixel - lowsample * master_edl->local_session->zoom_y / 2);
			int y1 = next_y1;
			int y2 = next_y2;

// A different algorithm has to be used if it's 1 sample per pixel and the
// index is used.  Now the min and max values are equal so we join the max samples.
			pixmap->canvas->set_color(theme_global->audio_color);
			if(master_edl->local_session->zoom_time * edlsession->sample_rate == 1)
			{
				pixmap->canvas->draw_line(x1 + x - 1, prev_y1, x1 + x, y1, pixmap);
			}
			else
			{
// Extend line height if it doesn't connect to previous line
				if(!first_frame)
				{
					if(y1 > prev_y2) y1 = prev_y2 + 1;
					if(y2 < prev_y1) y2 = prev_y1 - 1;
				}
				else
				{
					first_frame = 0;
				}
				pixmap->canvas->draw_line(x1 + x, y1, x1 + x, y2, pixmap);
			}
			current_frame -= index_frames_per_pixel;
			x1++;
			prev_y1 = next_y1;
			prev_y2 = next_y2;
			highsample = buffer[bufferposition];
			lowsample = buffer[bufferposition + 1];
		}

		current_frame++;
		highsample = MAX(highsample, buffer[bufferposition]);
		lowsample = MIN(lowsample, buffer[bufferposition + 1]);
	}

// Get last column
	if(current_frame)
	{
		y1 = round(center_pixel - highsample * master_edl->local_session->zoom_y / 2);
		y2 = round(center_pixel - lowsample * master_edl->local_session->zoom_y / 2);
		pixmap->canvas->draw_line(x1 + x, y1, x1 + x, y2, pixmap);
	}

	if(!buffer_shared)
		delete [] buffer;
	return 0;
}

void IndexFile::close_index()
{
	if(file)
	{
		fclose(file);
		file = 0;
	}
}

void IndexFile::remove_index()
{
	if(status == INDEX_READY || status == INDEX_NOTTESTED)
	{
		close_index();
		remove(index_filename);
		status = INDEX_NOTTESTED;
	}
}

int IndexFile::read_info()
{
	if(status == INDEX_NOTTESTED)
	{
// read start of index data
		if(fread((char*)&start, sizeof(off_t), 1, file) < 1)
			return 1;
// read test_asset info from index
		char *data = new char[start];

		if(fread(data, start - sizeof(off_t), 1, file))
		{
			FileXML xml;

			data[start - sizeof(off_t)] = 0;
			xml.read_from_string(data);
			asset->read(&xml);
			delete [] data;
			if(asset->format == FILE_UNKNOWN)
				return 1;
		}
		else
			return 1;
	}
	return 0;
}

void IndexFile::dump(int indent)
{
	const char *stxt;
	int channels = asset->streams[stream].channels;

	printf("%*sIndexFile %p dump:\n", indent, "", this);

	indent += 2;

	switch(status)
	{
	case INDEX_READY:
		stxt = "Ready";
		break;
	case INDEX_NOTTESTED:
		stxt = "Not Tested";
		break;
	case INDEX_BUILDING:
		stxt = "Building";
		break;
	case INDEX_TOOSMALL:
		stxt = "Too Small";
		break;
	default:
		stxt = "Unk";
		break;
	}
	if(index_filename)
		printf("'%s'\n", index_filename);
	printf("%*sstream %d start %zu '%s' zoom %d channels %d\n", indent, "",
		stream, start, stxt, zoom, channels);

	for(int i = 0; i < channels; i++)
		printf("%*s%2d %10d %10d\n", indent + 2, "",
			i, offsets[i], sizes[i]);
}
