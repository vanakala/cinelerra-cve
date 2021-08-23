// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filelist.h"
#include "interlacemodes.h"
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
}
int FileList::open_file(int open_mode)
{
	writing = open_mode & FILE_OPEN_WRITE;
	int result = 0;

// skip header for write
	if(open_mode & FILE_OPEN_WRITE)
	{
// Frame files are created in write_frame and list index is created when
// file is closed.
// Look for the starting number in the path but ignore the starting character
// and total digits since these are used by the header.
		Render::get_starting_number(asset->path, 
			first_number,
			number_start, 
			number_digits);
		path_list.remove_all_objects();
		writer = new FrameWriter(this, 
			asset->format == list_type ? file->cpus : 1);
	}
	else
	if(open_mode & FILE_OPEN_READ)
	{
// Determine type of file.
// Header isn't used for background rendering, in which case everything known
// by the file encoder is known by the decoder.
		if(asset->use_header)
		{
			FILE *stream = fopen(asset->path, "rb");
			if(stream)
			{
				char string[BCTEXTLEN];
				if(fread(string, strlen(list_prefix), 1, stream) < 1)
					result = 1;
				fclose(stream);
				if(result) return 1;

				if(!strncasecmp(string, list_prefix, strlen(list_prefix)))
				{
					asset->format = list_type;

// Open index here or get frame size from file.
					result = read_list_header();
					if(!result) result = read_frame_header(path_list.values[0]);
				}
				else
				{
					asset->format = frame_type;
					result = read_frame_header(asset->path);
					asset->set_single_image();
				}
			}
		}
		else
		{
			Render::get_starting_number(asset->path, 
				first_number,
				number_start, 
				number_digits,
				6);
		}
		if(asset->nb_streams)
		{
			struct streamdesc *sdsc = &asset->streams[0];

			switch(frame_type)
			{
			case FILE_JPEG:
				strcpy(sdsc->codec, "jpeg");
				break;
			case FILE_PNG:
				strcpy(sdsc->codec, "png");
				break;
			case FILE_TIFF:
				strcpy(sdsc->codec, "tiff");
				break;
			case FILE_TGA:
				break;
			default:
				sdsc->codec[0] = 0;
				break;
			}
			strcpy(asset->vcodec, sdsc->codec);
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
		if(writing && asset->use_header) write_list_header();
		path_list.remove_all_objects();
	}
	delete data;
	data = 0;
	delete writer;
	writer = 0;
}

void FileList::write_list_header()
{
	FILE *stream = fopen(asset->path, "w");
// Use sprintf instead of fprintf for VFS.
	char string[BCTEXTLEN];
	sprintf(string, "%s\n", list_prefix);
	fwrite(string, strlen(string), 1, stream);
	sprintf(string, "# First line is always %s\n", list_prefix);
	fwrite(string, strlen(string), 1, stream);
	sprintf(string, "# Frame rate:\n");
	fwrite(string, strlen(string), 1, stream);
	sprintf(string, "%f\n", asset->frame_rate);
	fwrite(string, strlen(string), 1, stream);
	sprintf(string, "# Width:\n");
	fwrite(string, strlen(string), 1, stream);
	sprintf(string, "%d\n", asset->width);
	fwrite(string, strlen(string), 1, stream);
	sprintf(string, "# Height:\n");
	fwrite(string, strlen(string), 1, stream);
	sprintf(string, "%d\n", asset->height);
	fwrite(string, strlen(string), 1, stream);
	sprintf(string, "# List of image files follows\n");
	fwrite(string, strlen(string), 1, stream);

	for(int i = 0; i < path_list.total; i++)
		fprintf(stream, "%s\n", path_list.values[i]);

	fclose(stream);
}

int FileList::read_list_header()
{
	char string[BCTEXTLEN], *new_entry;
	double frame_rate = 0;
	int width = 0;
	int height = 0;

	FILE *stream = fopen(asset->path, "r");

	if(stream)
	{
// Get information about the frames
		do
		{
			if(fgets(string, BCTEXTLEN, stream) == NULL)
				return 1;
		}while(string[0] == '#' || string[0] == ' ' || isalpha(string[0]));
		frame_rate = atof(string);

		do
		{
			if(fgets(string, BCTEXTLEN, stream) == NULL)
				return 1;
		}while(string[0] == '#' || string[0] == ' ');
		width = atoi(string);

		do
		{
			if(fgets(string, BCTEXTLEN, stream) == NULL)
				return 1;
		}while(string[0] == '#' || string[0] == ' ');
		height = atoi(string);

		asset->interlace_mode = BC_ILACE_MODE_UNDETECTED;  // May be good to store the info in the list?
		asset->audio_data = 0;

// Get all the paths
		while(!feof(stream))
		{
			if(fgets(string, BCTEXTLEN, stream) &&
				strlen(string) && string[0] != '#' && string[0] != ' ' && !feof(stream))
			{
				string[strlen(string) - 1] = 0;
				path_list.append(new_entry = new char[strlen(string) + 1]);
				strcpy(new_entry, string);
			}
		}
		asset->file_length = ftell(stream);
		fclose(stream);

		if(!asset->nb_streams)
		{
// Fill stream info
			struct streamdesc *sdsc = &asset->streams[0];
			sdsc->channels = 1;
			sdsc->width = width;
			sdsc->height = height;
			sdsc->length = path_list.total;
			sdsc->frame_rate = frame_rate;
			sdsc->end = (ptstime)path_list.total / frame_rate;
			sdsc->sample_aspect_ratio = 1;
			sdsc->options = STRDSC_VIDEO;
			asset->nb_streams = 1;
			asset->last_active = 0;
			asset->set_video_stream(0);
			asset->audio_streamno = 0;
		}
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

	current_frame = (frame->get_source_pts() + FRAME_OVERLAP) * asset->frame_rate;
	if(current_frame < 0 || 
		(asset->use_header && current_frame >= path_list.total &&
			asset->format == list_type))
		goto noframe;

	if(asset->format == list_type)
	{
		char *path;
		if(asset->use_header)
		{
			path = path_list.values[current_frame];
		}
		else
		{
			path = calculate_path(current_frame, string);
		}

		strcpy(string, path);

		if(!(fp = fopen(string, "rb")))
		{
			errormsg("Error while opening \"%s\" for reading. \n%m\n", string);
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
		frame->set_duration((ptstime)1 / asset->frame_rate);
		frame->set_source_pts((ptstime)current_frame / asset->frame_rate);
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
			errormsg("Error while opening \"%s\" for reading. \n%m\n", asset->path);
			goto noframe;
		}
		frame->set_source_pts(0);
		if(mwindow_global)
		{
			if(edlsession->si_useduration)
				frame->set_duration(edlsession->si_duration);
			else
				frame->set_duration((ptstime)1 / edlsession->frame_rate);
		}
	}
	frame->set_frame_number(current_frame);
	return result;

emptyfile:
	fclose(fp);
	errormsg("Error while opening \"%s\" for reading. \n%m\n", asset->path);
noframe:
	frame->clear_frame();
	frame->set_duration(1 / asset->frame_rate);
	frame->set_frame_number(-1);
	return 1;
}

int FileList::write_frames(VFrame ***frames, int len)
{
	return_value = 0;

	if(frames[0][0]->get_color_model() == BC_COMPRESSED)
	{
		for(int i = 0; i < asset->layers && !return_value; i++)
		{
			for(int j = 0; j < len && !return_value; j++)
			{
				VFrame *frame = frames[i][j];
				char *path = create_path(frame->get_frame_number());

				FILE *fd = fopen(path, "wb");
				if(fd)
				{
					return_value = !fwrite(frames[i][j]->get_data(),
						frames[i][j]->get_compressed_size(),
						1,
						fd);

					fclose(fd);
				}
				else
				{
					errormsg("Error while opening \"%s\" for writing. \n%m\n", asset->path);
					return_value++;
				}
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
// Synthesize filename.
// If a header is used, the filename number must be in a different location.
	if(asset->use_header)
	{
		int k;
		strcpy(string, asset->path);
		for(k = strlen(string) - 1; k > 0 && string[k] != '.'; k--)
			;
		if(k <= 0) k = strlen(string);

		sprintf(&string[k], "%06d%s", 
			number, 
			file_extension);
	}
	else
// Without a header, the original filename can be altered.
	{
		Render::create_filename(string, 
			asset->path, 
			number,
			number_digits,
			number_start);
	}

	return string;
}

char* FileList::create_path(int number_override)
{
	if(asset->format != list_type) return asset->path;

	table_lock->lock("FileList::create_path");

	char *path;
	char output[BCTEXTLEN];

	if(number_override >= path_list.total || !asset->use_header)
	{
		int number;
		number = number_override;

		if(!asset->use_header)
		{
			number += first_number;
		}
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
	if(data) result += data->get_compressed_allocated();
	return result;
}

int FileList::get_units()
{
	if(writer) return writer->get_total_clients();
	return 0;
}

FrameWriterUnit* FileList::get_unit(int number)
{
	if(writer) return (FrameWriterUnit*)writer->get_client(number);
	return 0;
}

FrameWriterPackage::FrameWriterPackage()
{
}

FrameWriterPackage::~FrameWriterPackage()
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
		errormsg("Failed to open \"%s\" for writing. \n%m\n", ptr->path);
		server->file->add_return_value(1);
		return;
	}


	int result = server->file->write_frame(ptr->input, output, this);

	if(!result) result = !fwrite(output->get_data(), output->get_compressed_size(), 1, file);
	fclose(file);
	server->file->add_return_value(result);
}


FrameWriter::FrameWriter(FileList *file, int cpus)
 : LoadServer(cpus, 0)
{
	this->file = file;
}

FrameWriter::~FrameWriter()
{
}

void FrameWriter::init_packages()
{
	for(int i = 0, layer = 0, number = 0; 
		i < get_total_packages(); 
		i++)
	{
		FrameWriterPackage *package = (FrameWriterPackage*)get_package(i);

		package->input = frames[layer][number];
		package->path = file->create_path(package->input->get_frame_number());
		number++;
		if(number >= len)
		{
			layer++;
			number = 0;
		}
	}
}

void FrameWriter::write_frames(VFrame ***frames, int len)
{
	this->frames = frames;
	this->len = len;
	set_package_count(len * file->asset->layers);
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
