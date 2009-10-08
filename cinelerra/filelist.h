
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

#ifndef FILELIST_H
#define FILELIST_H

#include "file.inc"
#include "filebase.h"
#include "filelist.inc"
#include "loadbalance.h"
#include "mutex.inc"
#include "vframe.inc"

// Any file which is a list of frames.
// FileList handles both frame files and indexes of frame files.






class FileList : public FileBase
{
public:
	FileList(Asset *asset, 
		File *file, 
		char *list_prefix,
		char *file_extension, 
		int frame_type,
		int list_type);
	virtual ~FileList();

// basic commands for every file interpreter
	int open_file(int rd, int wr);
	int close_file();

	char* calculate_path(int number, char *string);
	char* create_path(int number_override);
	void add_return_value(int amount);

	int read_list_header();
	virtual int read_frame_header(char *path) { return 1; };
	int read_frame(VFrame *frame);

// subclass returns whether the asset format is a list or single file
	virtual int read_frame(VFrame *frame, VFrame *data) { return 0; };
	virtual int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit) { return 0; };


	int write_list_header();
	int write_frames(VFrame ***frames, int len);
	VFrame* read_frame(int use_alpha, int use_float);
	virtual int64_t get_memory_usage();
// Get the total writer units for calculating memory usage
	int get_units();
// Get a writer unit for retrieving temporary usage.
	FrameWriterUnit* get_unit(int number);

	virtual FrameWriterUnit* new_writer_unit(FrameWriter *writer);

// Temp storage for compressed data
	VFrame *data;
// Storage for single frame
	VFrame *temp;

private:
	int read_raw(VFrame *frame, 
		float in_x1, float in_y1, float in_x2, float in_y2,
		float out_x1, float out_y1, float out_x2, float out_y2, 
		int alpha, int use_alpha, int use_float, int interpolate);
	int reset_parameters_derived();
	ArrayList<char*> path_list;     // List of files
	char *list_prefix;
	char *file_extension;
	int frame_type;
	int list_type;
	Mutex *table_lock;
	FrameWriter *writer;
	int return_value;
	int first_number;
	int number_start;
	int number_digits;
};




class FrameWriterPackage : public LoadPackage
{
public:
	FrameWriterPackage();
	~FrameWriterPackage();
	
	
	VFrame *input;
	
	char *path;
};




class FrameWriterUnit : public LoadClient
{
public:
	FrameWriterUnit(FrameWriter *server);
	virtual ~FrameWriterUnit();
	
	void process_package(LoadPackage *package);

	FrameWriter *server;
	VFrame *output;
};





class FrameWriter : public LoadServer
{
public:
	FrameWriter(FileList *file, int cpus);
	~FrameWriter();
	
	void write_frames(VFrame ***frames, int len);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	FileList *file;
	VFrame ***frames;
	int len;
};








#endif
