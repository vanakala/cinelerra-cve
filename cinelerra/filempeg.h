
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

#ifndef FILEMPEG_H
#define FILEMPEG_H

#include "condition.inc"
#include "file.inc"
#include "filebase.h"
#include "libmpeg3.h"
#include "thread.h"


class FileMPEG : public FileBase
{
public:
	FileMPEG(Asset *asset, File *file);
	~FileMPEG();

	static int check_sig(Asset *asset);

	int open_file(int rd, int wr);
	void close_file();
	int create_index();

	int get_index(const char *index_path);

	int read_frame(VFrame *frame);
	int read_aframe(AFrame *aframe);
	int prefer_samples_float();
	int to_mpeg_colormodel(int cmodel);

	int64_t get_memory_usage();

// Direct copy routines
	static int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
	static const char *strtocompression(const char *string);
	static const char *compressiontostr(const char *string);

private:
// File descriptor for decoder
	mpeg3_t *fd;

// Temp buffer
	float *temp_float;
	int temp_float_allocated;
};

#endif
