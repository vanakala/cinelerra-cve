
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
#include "file.h"
#include "filelist.h"
#include "guicast.h"
#include "interlacemodes.h"
#include "mutex.h"
#include "mwindow.inc"
#include "render.h"
#include "renderfarmfsserver.inc"
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
	char *list_prefix,
	char *file_extension, 
	int frame_type,
	int list_type)
 : FileBase(asset, file)
{
	reset_parameters();
	asset->video_data = 1;
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

int FileList::reset_parameters_derived()
{
	data = 0;
	writer = 0;
	temp = 0;
	first_number = 0;
}

int FileList::open_file(int rd, int wr)
{
	this->rd = rd;
	this->wr = wr;
	int result = 0;

// skip header for write
	if(wr)
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
	if(rd)
	{
// Determine type of file.
// Header isn't used for background rendering, in which case everything known
// by the file encoder is known by the decoder.
//printf("FileList::open_file 1 %d\n", asset->use_header);
		if(asset->use_header)
		{
			FILE *stream = fopen(asset->path, "rb");
			if(stream)
			{
				char string[BCTEXTLEN];
				fread(string, strlen(list_prefix), 1, stream);
				fclose(stream);

				if(!strncasecmp(string, list_prefix, strlen(list_prefix)))
				{

					asset->format = list_type;

// Open index here or get frame size from file.
					result = read_list_header();
					if(!result) result = read_frame_header(path_list.values[0]);
				}
				else
				{
//printf("FileList::open_file 2\n", asset->use_header);
					asset->format = frame_type;
					result = read_frame_header(asset->path);
					asset->layers = 1;
					if(!asset->frame_rate)
						asset->frame_rate = 1;
					asset->video_length = -1;
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
	}

	file->current_frame = 0;
// Compressed data storage
	data = new VFrame;

	return result;
}


int FileList::close_file()
{
//	path_list.total, asset->format, list_type, wr);
	if(asset->format == list_type && path_list.total)
	{
		if(wr && asset->use_header) write_list_header();
		path_list.remove_all_objects();
	}
	if(data) delete data;
	if(writer) delete writer;
	if(temp) delete temp;
	reset_parameters();

	FileBase::close_file();
	return 0;
}

int FileList::write_list_header()
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
	{
// Fix path for VFS but leave leading slash
		if(!strncmp(path_list.values[i], RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
			sprintf(string, "%s\n", path_list.values[i] + strlen(RENDERFARM_FS_PREFIX));
		else
			sprintf(string, "%s\n", path_list.values[i]);
		fwrite(string, strlen(string), 1, stream);
	}
	fclose(stream);
	return 0;
}

int FileList::read_list_header()
{
	char string[BCTEXTLEN], *new_entry;

	FILE *stream = fopen(asset->path, "r");
	
	
	if(stream)
	{
// Get information about the frames
		do
		{
			fgets(string, BCTEXTLEN, stream);
		}while(!feof(stream) && (string[0] == '#' || string[0] == ' ' || isalpha(string[0])));

// Don't want a user configured frame rate to get destroyed
		if(asset->frame_rate == 0)
			asset->frame_rate = atof(string);

		do
		{
			fgets(string, BCTEXTLEN, stream);
		}while(!feof(stream) && (string[0] == '#' || string[0] == ' '));
		asset->width = atol(string);

		do
		{
			fgets(string, BCTEXTLEN, stream);
		}while(!feof(stream) && (string[0] == '#' || string[0] == ' '));
		asset->height = atol(string);

		asset->interlace_mode = BC_ILACE_MODE_UNDETECTED;  // May be good to store the info in the list?
		asset->layers = 1;
		asset->audio_data = 0;
		asset->video_data = 1;

// Get all the paths
		while(!feof(stream))
		{
			fgets(string, BCTEXTLEN, stream);
			if(strlen(string) && string[0] != '#' && string[0] != ' ' && !feof(stream))
			{
				string[strlen(string) - 1] = 0;
				path_list.append(new_entry = new char[strlen(string) + 1]);
				strcpy(new_entry, string);
			}
		}

//for(int i = 0; i < path_list.total; i++) printf("%s\n", path_list.values[i]);
		fclose(stream);
		asset->video_length = path_list.total;
	}
	else
		return 1;

	return 0;
}

int FileList::read_frame(VFrame *frame)
{
	int result = 0;
	if(file->current_frame < 0 || 
		(asset->use_header && file->current_frame >= path_list.total &&
			asset->format == list_type))
		return 1;

	if(asset->format == list_type)
	{
		char string[BCTEXTLEN];
		char *path;
		if(asset->use_header)
		{
			path = path_list.values[file->current_frame];
		}
		else
		{
			path = calculate_path(file->current_frame, string);
		}
		FILE *in;


// Fix path for VFS
		if(!strncmp(asset->path, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
			sprintf(string, "%s%s", RENDERFARM_FS_PREFIX, path);
		else
			strcpy(string, path);

		if(!(in = fopen(string, "rb")))
		{
			eprintf("Error while opening \"%s\" for reading. \n%m\n", string);
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
					fread(frame->get_data(), ostat.st_size, 1, in);
					break;
				default:
					data->allocate_compressed_data(ostat.st_size);
					data->set_compressed_size(ostat.st_size);
					fread(data->get_data(), ostat.st_size, 1, in);
					result = read_frame(frame, data);
					break;
			}


			fclose(in);
		}
	}
	else
	{
// Allocate and decompress once into temporary
//printf("FileList::read_frame %d\n", frame->get_color_model());
		if(!temp || temp->get_color_model() != frame->get_color_model())
		{
			if(temp) delete temp;
			temp = 0;
		
			FILE *fd = fopen(asset->path, "rb");
			if(fd)
			{
				struct stat ostat;
				stat(asset->path, &ostat);

				switch(frame->get_color_model())
				{
					case BC_COMPRESSED:
						frame->allocate_compressed_data(ostat.st_size);
						frame->set_compressed_size(ostat.st_size);
						fread(frame->get_data(), ostat.st_size, 1, fd);
						break;
					default:
						data->allocate_compressed_data(ostat.st_size);
						data->set_compressed_size(ostat.st_size);
						fread(data->get_data(), ostat.st_size, 1, fd);
						temp = new VFrame(0, 
							asset->width, 
							asset->height, 
							frame->get_color_model());
						read_frame(temp, data);
						break;
				}

				fclose(fd);
			}
			else
			{
				eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
				result = 1;
			}
		}

		if(!temp) return result;

// printf("FileList::read_frame frame=%d temp=%d\n", 
// frame->get_color_model(),
// temp->get_color_model());
		if(frame->get_color_model() == temp->get_color_model())
		{
			frame->copy_from(temp);
		}
		else
		{
// Never happens
			cmodel_transfer(frame->get_rows(), /* Leave NULL if non existent */
				temp->get_rows(),
				frame->get_y(), /* Leave NULL if non existent */
				frame->get_u(),
				frame->get_v(),
				temp->get_y(), /* Leave NULL if non existent */
				temp->get_u(),
				temp->get_v(),
				0,        /* Dimensions to capture from input frame */
				0, 
				asset->width, 
				asset->height,
				0,       /* Dimensions to project on output frame */
				0, 
				asset->width, 
				asset->height,
				temp->get_color_model(), 
				frame->get_color_model(),
				0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
				temp->get_w(),       /* For planar use the luma rowspan */
				frame->get_w());
		}
	}


//printf("FileList::read_frame 5 %d\n", result);


	return result;
}

int FileList::write_frames(VFrame ***frames, int len)
{
	return_value = 0;

//printf("FileList::write_frames 1\n");
	if(frames[0][0]->get_color_model() == BC_COMPRESSED)
	{
		for(int i = 0; i < asset->layers && !return_value; i++)
		{
			for(int j = 0; j < len && !return_value; j++)
			{
				VFrame *frame = frames[i][j];
				char *path = create_path(frame->get_number());

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
					eprintf("Error while opening \"%s\" for writing. \n%m\n", asset->path);
					return_value++;
				}
			}
		}
	}
	else
	{
//printf("FileList::write_frames 2\n");
		writer->write_frames(frames, len);
//printf("FileList::write_frames 100\n");
	}
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



	char *path = "";
	char output[BCTEXTLEN];
	if(file->current_frame >= path_list.total || !asset->use_header)
	{
		int number;
		if(number_override < 0)
			number = file->current_frame++;
		else
		{
			number = number_override;
			file->current_frame++;
		}

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
		path = path_list.values[file->current_frame];
	}


	table_lock->unlock();
	
	return path;
}

FrameWriterUnit* FileList::new_writer_unit(FrameWriter *writer)
{
	return new FrameWriterUnit(writer);
}

int64_t FileList::get_memory_usage()
{
	int64_t result = 0;
	if(data) result += data->get_compressed_allocated();
	if(temp) result += temp->get_data_size();
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
//printf("FrameWriterUnit::process_package 1\n");
	FrameWriterPackage *ptr = (FrameWriterPackage*)package;

	FILE *file;

//printf("FrameWriterUnit::process_package 2 %s\n", ptr->path);
	if(!(file = fopen(ptr->path, "wb")))
	{
		eprintf("Error while opening \"%s\" for writing. \n%m\n", ptr->path);
		return;
	}
//printf("FrameWriterUnit::process_package 3");


	int result = server->file->write_frame(ptr->input, output, this);
	
//printf("FrameWriterUnit::process_package 4 %s %d\n", ptr->path, output->get_compressed_size());
	if(!result) result = !fwrite(output->get_data(), output->get_compressed_size(), 1, file);
//TRACE("FrameWriterUnit::process_package 4");
	fclose(file);
//TRACE("FrameWriterUnit::process_package 5");

	server->file->add_return_value(result);
//TRACE("FrameWriterUnit::process_package 6");
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
		package->path = file->create_path(package->input->get_number());
// printf("FrameWriter::init_packages 1 %p %d %s\n", 
// package->input,
// package->input->get_number(), 
// package->path);
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



