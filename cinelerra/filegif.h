
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

#ifndef FILEGIF_H
#define FILEGIF_H

#include "file.inc"
#include "filebase.h"
#include "vframe.inc"

// This header file is representative of any single frame file format.

class FileGIF : public FileBase
{
public:
	FileGIF(Asset *asset);
	~FileGIF();

// basic commands for every file interpreter
	int open_file(int rd, int wr);
	int close_file_derived();
	int64_t get_video_length();
	int64_t get_memory_usage();

	int read_header();
	VFrame* read_frame(int use_alpha, int use_float);

private:
	int read_raw();
	int reset_parameters_derived();

// specific to GIF
	int import_row(VPixel *output, unsigned char *row_pointer);

// routines for all image files
// frame to return through read_frame
	VFrame *data;
};


#endif
