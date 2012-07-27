
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * Copyright (C) 2012 Einar Rünkaru <einarry at smail dot ee>
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

#ifndef FILETOC_H
#define FILETOC_H

#include "bcwindowbase.inc"
#include "datatype.h"
#include "maxchannels.h"
#include "mainprogress.inc"
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#define TOCFILE_EXT           ".toc"

#define TOCFILE_PREFIX         0x20434f54
#define TOCFILE_VERSION        0x00001300

#include "filebase.inc"
#include "filetoc.inc"

#define MPEG3_AUDIO_CHUNKSIZE 0x10000

#define ITEM_BLOCK 4096

class FileTOC
{
public:
	FileTOC(FileBase *media, const char *dirpath, const char *filepath,
		off_t length, time_t mtime);
	~FileTOC();

	int init_tocfile(int ftype);
	// Returns non-zero if user has canceled toc generation
	int append_item(posnum index, off_t offset);
	stream_item *get_item(int stream, posnum ix);
	int id_to_index(int id);
	void dump(int offsets);

	stream_params toc_streams[MAXCHANNELS];
	int file_type;
	int num_streams;

private:
	void put_int32(uint32_t x);
	void put_int64(uint64_t x);
	uint32_t read_int32();
	uint64_t read_int64();

	FileBase *media;
	FILE *tocfile;
	MainProgressBar *progress;
	int canceled;
	int streamnum;
	off_t length;
	time_t mtime;
	const char *filepath;
	int fileio_err;
	char toc_filename[BCTEXTLEN];
	int items_allocated;
	int max_items;
	stream_item *items;
};

#endif
