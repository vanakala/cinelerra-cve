// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filelist.h"
#include "filesystem.h"
#include "interlacemodes.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.h"
#include "render.h"
#include "vframe.h"
#include "mainerror.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct list_types FileList::list_types[] =
{
	{ FILE_JPEG, FILE_JPEG_LIST, "JPEGLIST", ".jpg" },
	{ FILE_PNG, FILE_PNG_LIST, "PNGLIST", ".png" },
	{ FILE_TGA, FILE_TGA_LIST, "TGALIST", ".tga" },
	{ FILE_TIFF, FILE_TIFF_LIST, "TIFFLIST", ".tif" },
	{ 0, 0, 0, 0 }
};


FileList::FileList(Asset *asset, 
	File *file, 
	const char *list_prefix,
	const char *file_extension, 
	int frame_type,
	int list_type)
 : FileBase(asset, file)
{
	data = 0;
	writer = 0;
	writing = 0;
	first_number = 0;
	renderpath = 0;
	path_list.set_array_delete();
	this->list_prefix = list_prefix;
	this->file_extension = file_extension;
	this->frame_type = frame_type;
	this->list_type = list_type;
	table_lock = new Mutex("FileList::table_lock");
}

FileList::~FileList()
{
	close_file();
	delete table_lock;
	delete [] renderpath;
}

int FileList::probe_list(Asset *asset)
{
	FILE *stream;
	char string[BCTEXTLEN];
	struct list_types *list_type = 0;
	double frame_rate = 0;
	int width = 0;
	int height = 0;
	int count = 0;

	if(asset->format != FILE_UNKNOWN && asset->nb_streams)
		return 0;

	if(!(stream = fopen(asset->path, "rb")))
		return -1;

	if(!fgets(string, BCTEXTLEN, stream))
	{
		fclose(stream);
		return -1;
	}

	for(int i = 0; list_types[i].list_prefix; i++)
	{
		if(!strncmp(string, list_types[i].list_prefix,
			strlen(list_types[i].list_prefix)))
		{
			list_type = &list_types[i];
			break;
		}
	}

	if(!list_type)
	{
		fclose(stream);
		return 1;
	}
	asset->format = list_type->list_type;

	for(int i = 0; fgets(string, BCTEXTLEN, stream);)
	{
		if(string[0] == '#' || string[0] == '\n')
			continue;
		if(i < 3)
		{
			if(isalpha(string[0]))
				continue;
			if(i == 0)
				frame_rate = atof(string);
			if(i == 1)
				width = atoi(string);
			if(i == 2)
				height = atoi(string);
		}
		else
			count++;
		i++;
	}
	asset->file_length = ftell(stream);
	fclose(stream);

	if(width < MIN_FRAME_WIDTH || width > MAX_FRAME_WIDTH ||
		height < MIN_FRAME_HEIGHT || height > MAX_FRAME_HEIGHT)
	{
		errormsg(_("Image list '%s' frame size is outside limits"),
				asset->path);
		return 1;
	}

	if(frame_rate < MIN_FRAME_RATE || frame_rate > MAX_FRAME_RATE)
	{
		errormsg(_("Image list '%s' frame rate is outside limits"),
			asset->path);
		return 1;
	}

	struct streamdesc *sdsc = &asset->streams[0];

	sdsc->channels = 1;
	sdsc->width = width;
	sdsc->height = height;
	sdsc->length = count;
	sdsc->frame_rate = frame_rate;
	sdsc->end = (ptstime)count / frame_rate;
	sdsc->sample_aspect_ratio = 1;
	sdsc->options = STRDSC_VIDEO;
	sdsc->start = 0;
	sdsc->end = count / frame_rate;
	asset->nb_streams = 1;
	asset->set_base_pts(0);

	return 0;
}

int FileList::open_file(int open_mode, int streamix, const char *filepath)
{
	writing = open_mode & FILE_OPEN_WRITE;
	int result = 0;

// skip header for write
	if(writing)
	{
		renderpath = new char[BCTEXTLEN];
		if(filepath)
			strcpy(renderpath, filepath);
		else
			strcpy(renderpath, asset->path);
// Frame files are created in write_frame and list index is created when
// file is closed.
// Look for the starting number in the path but ignore the starting character
// and total digits since these are used by the header.
		Render::get_starting_number(renderpath,
			first_number,
			number_start, 
			number_digits);
		path_list.remove_all_objects();
		writer = new FrameWriter(this, 
			asset->format == list_type ? file->cpus : 1);
	}
	else if(open_mode & FILE_OPEN_READ)
	{
		switch(asset->format)
		{
		case FILE_JPEG_LIST:
		case FILE_PNG_LIST:
		case FILE_TGA_LIST:
		case FILE_TIFF_LIST:
			if(!(result = read_list_header()))
				result = read_frame_header(path_list.values[0]);
			break;

		case FILE_JPEG:
		case FILE_PNG:
		case FILE_TGA:
		case FILE_TIFF:
			result = read_frame_header(asset->path);
			asset->set_single_image();
			break;

		default:
			result = 1;
			break;
		}
	}
// Compressed data storage
	data = new VFrame;
	return result;
}

void FileList::close_file()
{
	if(asset->format == list_type && path_list.total)
	{
		if(writing && !(asset->strategy & RENDER_BRENDER))
			write_list_header();
		path_list.remove_all_objects();
	}
	delete data;
	data = 0;
	delete writer;
	writer = 0;
}

void FileList::write_list_header()
{
	FILE *stream = fopen(renderpath, "w");
	FileSystem fs;
	int dir_len;
	char *path;
	char output_directory[BCTEXTLEN];
	int streamix = asset->get_stream_ix(STRDSC_VIDEO);

	if(stream)
	{
		fs.extract_dir(output_directory, renderpath);
		dir_len = strlen(output_directory);

		fprintf(stream, "%s\n", list_prefix);
		fprintf(stream, "# First line is always %s\n", list_prefix);
		fprintf(stream, "# Frame rate:\n%f\n", asset->streams[streamix].frame_rate);
		fprintf(stream, "# Width:\n%d\n", asset->streams[streamix].width);
		fprintf(stream, "# Height:\n%d\n", asset->streams[streamix].height);
		fprintf(stream, "# List of image files follows\n");

		for(int i = 0; i < path_list.total; i++)
		{
			path = path_list.values[i];
			if(strncmp(path, output_directory, dir_len) == 0)
				path = &path[dir_len];
			fprintf(stream, "%s\n", path);
		}
		fclose(stream);
	}
	else
		errormsg(_("Can't create '%s': %m\n"), renderpath);
}

int FileList::read_list_header()
{
	char string[BCTEXTLEN], *new_entry;
	char path[BCTEXTLEN];

	FILE *stream = fopen(asset->path, "r");

	if(stream)
	{
		for(int i = 0; fgets(string, BCTEXTLEN, stream);)
		{
			if(string[0] == '#' || string[0] == '\n')
				continue;

			if(i < 3)
			{
				if(isalpha(string[0]))
					continue;
				i++;
			}
			else
			{
				int length = strlen(string);
				string[length - 1] = 0;

				if(string[0] != '/')
				{
					char new_path[BCTEXTLEN];
					FileSystem fs;

					strcpy(new_path, string);
					fs.extract_dir(string, asset->path);
					strcat(string, new_path);
				}
				path_list.append(new_entry = new char[strlen(string) + 1]);
				strcpy(new_entry, string);
			}
		}
		fclose(stream);
	}
	else
		return 1;

	return 0;
}

int FileList::read_frame(VFrame *frame)
{
	int result = 0;
	framenum current_frame;
	FILE *fp;
	char string[BCTEXTLEN];

	current_frame = (frame->get_source_pts() + FRAME_OVERLAP) *
		asset->streams[0].frame_rate;
	if(current_frame < 0 ||
			(!(asset->strategy & RENDER_BRENDER) &&
			current_frame >= path_list.total &&
			asset->format == list_type))
		goto noframe;

	if(asset->format == list_type)
	{
		char *path;

		if(!(asset->strategy & RENDER_BRENDER))
			path = path_list.values[current_frame];
		else
			path = calculate_path(current_frame, string);

		strcpy(string, path);

		if(!(fp = fopen(string, "rb")))
		{
			errormsg(_("Error while opening \"%s\" for reading. \n%m\n"), string);
		}
		else
		{
			struct stat ostat;
			stat(string, &ostat);

			switch(frame->get_color_model())
			{
			case BC_COMPRESSED:
				frame->allocate_compressed_data(ostat.st_size);
				frame->set_compressed_size(ostat.st_size);
				if(fread(frame->get_data(), ostat.st_size, 1, fp) < 1)
					goto emptyfile;
				break;
			default:
				data->allocate_compressed_data(ostat.st_size);
				data->set_compressed_size(ostat.st_size);
				if(fread(data->get_data(), ostat.st_size, 1, fp) < 1)
					goto emptyfile;
				result = read_frame(frame, data);
				break;
			}
			fclose(fp);
		}
		frame->set_duration((ptstime)1 / asset->streams[0].frame_rate);
		frame->set_source_pts((ptstime)current_frame / asset->streams[0].frame_rate);
	}
	else
	{
		current_frame = 0;

		if(fp = fopen(asset->path, "rb"))
		{
			struct stat ostat;
			stat(asset->path, &ostat);

			switch(frame->get_color_model())
			{
			case BC_COMPRESSED:
				frame->allocate_compressed_data(ostat.st_size);
				frame->set_compressed_size(ostat.st_size);
				if(fread(frame->get_data(), ostat.st_size, 1, fp) < 1)
					goto emptyfile;
				break;
			default:
				data->allocate_compressed_data(ostat.st_size);
				data->set_compressed_size(ostat.st_size);
				if(fread(data->get_data(), ostat.st_size, 1, fp) < 1)
					goto emptyfile;
				read_frame(frame, data);
				break;
			}
			asset->file_length = ostat.st_size;
			fclose(fp);
		}
		else
		{
			errormsg(_("Error while opening \"%s\" for reading. \n%m\n"), asset->path);
			goto noframe;
		}
		frame->set_source_pts(0);

		if(edlsession->si_useduration)
			frame->set_duration(edlsession->si_duration);
		else
			frame->set_duration((ptstime)1 / edlsession->frame_rate);
	}
	frame->set_frame_number(current_frame);
	return result;

emptyfile:
	fclose(fp);
	errormsg(_("Error while opening \"%s\" for reading. \n"), asset->path);
noframe:
	frame->clear_frame();
	frame->set_duration(1 / asset->streams[0].frame_rate);
	frame->set_frame_number(-1);
	return 1;
}

int FileList::write_frames(VFrame **frames, int len)
{
	int streamix = asset->get_stream_ix(STRDSC_VIDEO);

	return_value = 0;

	if(frames[0]->get_color_model() == BC_COMPRESSED)
	{
		for(int j = 0; j < len && !return_value; j++)
		{
			VFrame *frame = frames[j];
			char *path = create_path(frame->get_frame_number());

			FILE *fd = fopen(path, "wb");
			if(fd)
			{
				return_value = !fwrite(frames[j]->get_data(),
					frames[j]->get_compressed_size(),
					1, fd);

				fclose(fd);
			}
			else
			{
				errormsg(_("Error while opening \"%s\" for writing. \n%m\n"), renderpath);
				return_value++;
			}
		}
	}
	else
		writer->write_frames(frames, len);
	return return_value;
}

void FileList::add_return_value(int amount)
{
	table_lock->lock("FileList::add_return_value");
	return_value += amount;
	table_lock->unlock();
}

char* FileList::calculate_path(int number, char *string)
{
	char *path;

	if(writing)
		path = renderpath;
	else
		path = asset->path;

	if(!(asset->strategy & RENDER_BRENDER))
	{
		int k;
		strcpy(string, path);
		for(k = strlen(string) - 1; k > 0 && string[k] != '.'; k--);

		if(k <= 0)
			k = strlen(string);

		sprintf(&string[k], "%06d%s", number, file_extension);
	}
	else
// Without a header, the original filename can be altered.
	{
		Render::create_filename(string, 
			path,
			number,
			number_digits,
			number_start);
	}

	return string;
}

char* FileList::create_path(int number_override)
{
	if(asset->format != list_type)
		return renderpath ? renderpath : asset->path;

	table_lock->lock("FileList::create_path");

	char *path;
	char output[BCTEXTLEN];

	if(number_override >= path_list.total || (asset->strategy & RENDER_BRENDER))
	{
		int number = number_override;

		if(asset->strategy & RENDER_BRENDER)
			number += first_number;

		calculate_path(number, output);

		path = new char[strlen(output) + 1];
		strcpy(path, output);
		path_list.append(path);
	}
	else
	{
// Overwrite an old path
		path = path_list.values[number_override];
	}

	table_lock->unlock();

	return path;
}

FrameWriterUnit* FileList::new_writer_unit(FrameWriter *writer)
{
	return new FrameWriterUnit(writer);
}

size_t FileList::get_memory_usage()
{
	size_t result = 0;

	if(data)
		result += data->get_compressed_allocated();
	return result;
}

int FileList::get_units()
{
	if(writer)
		return writer->get_total_clients();
	return 0;
}

FrameWriterUnit* FileList::get_unit(int number)
{
	if(writer)
		return (FrameWriterUnit*)writer->get_client(number);
	return 0;
}

FrameWriterPackage::FrameWriterPackage()
{
}


FrameWriterUnit::FrameWriterUnit(FrameWriter *server)
 : LoadClient(server)
{
// Don't use server here since subclasses call this with no server.
	this->server = server;
	output = new VFrame;
}

FrameWriterUnit::~FrameWriterUnit()
{
	delete output;
}

void FrameWriterUnit::process_package(LoadPackage *package)
{
	FrameWriterPackage *ptr = (FrameWriterPackage*)package;

	FILE *file;

	if(!(file = fopen(ptr->path, "wb")))
	{
		errormsg(_("Failed to open \"%s\" for writing. \n%m\n"), ptr->path);
		server->file->add_return_value(1);
		return;
	}

	int result = server->file->write_frame(ptr->input, output, this);

	if(!result)
		result = !fwrite(output->get_data(), output->get_compressed_size(), 1, file);

	fclose(file);
	server->file->add_return_value(result);
}


FrameWriter::FrameWriter(FileList *file, int cpus)
 : LoadServer(cpus, 0)
{
	this->file = file;
}

void FrameWriter::init_packages()
{
	for(int i = 0, number = 0;
		i < get_total_packages(); i++)
	{
		FrameWriterPackage *package = (FrameWriterPackage*)get_package(i);

		package->input = frames[number];
		package->path = file->create_path(package->input->get_frame_number());
		number++;
	}
}

void FrameWriter::write_frames(VFrame **frames, int len)
{
	this->frames = frames;
	this->len = len;
	set_package_count(len);
	process_packages();
}

LoadClient* FrameWriter::new_client()
{
	return file->new_writer_unit(this);
}

LoadPackage* FrameWriter::new_package()
{
	return new FrameWriterPackage;
}
