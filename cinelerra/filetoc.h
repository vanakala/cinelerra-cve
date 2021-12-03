// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru at gmail dot com>

#ifndef FILETOC_H
#define FILETOC_H

#include "bcwindowbase.inc"
#include "cinelerra.h"
#include "datatype.h"
#include "mainprogress.inc"
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

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
	// mdoffs is position in bytes (0..file legth for every stream)
	int append_item(posnum index, off_t offset, off_t mdoffs);
	stream_item *get_item(int stream, posnum ix, stream_item **nxitem);
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
