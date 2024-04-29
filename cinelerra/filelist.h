// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILELIST_H
#define FILELIST_H

#include "file.inc"
#include "filebase.h"
#include "filelist.inc"
#include "loadbalance.h"
#include "mutex.inc"
#include "vframe.inc"

class FrameWriter;
class FrameWriterUnit;

struct list_types
{
	int frame_type;
	int list_type;
	const char *list_prefix;
	const char *extension;
};

// Any file which is a list of frames.
// FileList handles both frame files and indexes of frame files.

class FileList : public FileBase
{
public:
	FileList(Asset *asset, 
		File *file, 
		const char *list_prefix,
		const char *file_extension, 
		int frame_type,
		int list_type);
	virtual ~FileList();

	static int probe_list(Asset *asset);
// basic commands for every file interpreter
	int open_file(int open_mode, int streamix, const char *filepath);
	void close_file();

	char* calculate_path(int number, char *string);
	char* create_path(int number_override);
	void add_return_value(int amount);

	int read_list_header();
	virtual int read_frame_header(const char *path) { return 1; };
	int read_frame(VFrame *frame);

// subclass returns whether the asset format is a list or single file
	virtual int read_frame(VFrame *frame, VFrame *data) { return 0; };
	virtual int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit) { return 0; };

	void write_list_header();
	int write_frames(VFrame **frames, int len);
	VFrame* read_frame(int use_alpha, int use_float);
	virtual size_t get_memory_usage();
// Get the total writer units for calculating memory usage
	int get_units();
// Get a writer unit for retrieving temporary usage.
	FrameWriterUnit* get_unit(int number);

	virtual FrameWriterUnit* new_writer_unit(FrameWriter *writer);

// Temp storage for compressed data
	VFrame *data;
// Render path
	char *renderpath;

private:
	ArrayList<char*> path_list;     // List of files
	const char *list_prefix;
	const char *file_extension;
	int frame_type;
	int list_type;
	Mutex *table_lock;
	FrameWriter *writer;
	int return_value;
	int first_number;
	int number_start;
	int number_digits;
	int writing;
	static struct list_types list_types[];
};


class FrameWriterPackage : public LoadPackage
{
public:
	FrameWriterPackage();

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

	void write_frames(VFrame **frames, int len);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	FileList *file;
	VFrame **frames;
	int len;
};

#endif
