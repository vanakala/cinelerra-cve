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
#include "bctimer.inc"
#include "tracks.inc"

class IndexFile
{
public:
	IndexFile();
	IndexFile(Asset *asset);
	~IndexFile();

	int open_index(Asset *asset);
	int create_index(Asset *asset, MainProgressBar *progress);
	void interrupt_index();
	static void delete_index(Preferences *preferences, Asset *asset);
	static void get_index_filename(char *source_filename, 
		const char *index_directory,
		char *index_filename, 
		const char *input_filename,
		int stream = -1,
		const char *extension = 0);
	void update_edl_asset();
	void redraw_edits(int force);
	int draw_index(ResourcePixmap *pixmap, Edit *edit, int x, int w);
	void close_index();
	void remove_index();
	int read_info();
	int write_info();

	char index_filename[BCTEXTLEN];
	char source_filename[BCTEXTLEN];
	Asset *asset;
	Timer *redraw_timer;

private:
	void update_mainasset();

	int open_file();
	int open_source(File *source);
	int get_required_scale(File *source);
	FILE *file;
	off_t file_length;   // Length of index file in bytes
	int interrupt_flag;    // Flag set when index building is interrupted
};

#endif
