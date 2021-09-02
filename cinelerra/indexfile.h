// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef INDEXFILE_H
#define INDEXFILE_H

#include "asset.inc"
#include "edit.inc"
#include "file.inc"
#include "indexthread.inc"
#include "mainprogress.inc"
#include "preferences.inc"
#include "resourcepixmap.inc"
#include "bctimer.h"
#include "tracks.inc"

class IndexFile
{
public:
	IndexFile();
	~IndexFile();

	int open_index(Asset *asset, int stream);
	void allocate_buffer(int index_size);
	void release_buffer();
	int create_index(Asset *asset, int stream, MainProgressBar *progress);
	void interrupt_index();
	void delete_index();
	static void get_index_filename(char *source_filename, 
		const char *index_directory,
		char *index_filename, 
		const char *input_filename,
		int stream = -1,
		const char *extension = 0);
	void redraw_edits(int force);
	int draw_index(ResourcePixmap *pixmap, Edit *edit, int x, int w);
	void close_index();
	void remove_index();
	int read_info();
	int write_info();
	void dump(int index);

	char *index_filename;
	Asset *asset;
	int stream;
	int status;
	int zoom;
	// byte start of index data in the index file
	off_t start;
	off_t index_end, old_index_end;
	// Offsets and sizes of channels in index buffer
	int offsets[MAXCHANNELS];
	int sizes[MAXCHANNELS];
	Timer redraw_timer;
// [ index channel      ][ index channel      ]
// [high][low][high][low][high][low][high][low]
	float *index_buffer;

private:
	int open_file();
	int open_source(File *source);
	int get_required_scale(File *source);

	FILE *file;
	off_t file_length;   // Length of index file in bytes
	int interrupt_flag;    // Flag set when index building is interrupted
	int buffer_allocated;
};

#endif
