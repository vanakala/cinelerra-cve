// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILETIFF_H
#define FILETIFF_H

#include <stdlib.h>
#include "datatype.h"
#include "file.inc"
#include "filelist.h"
#include "mutex.inc"
#include <tiffio.h>
#include "vframe.inc"

// This header file is representative of any single frame file format.

class FileTIFF : public FileList
{
public:
	FileTIFF(Asset *asset, File *file);

	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int options);
	static int check_sig(Asset *asset);
	static const char* compression_name(int value);
	static const char* cmodel_name(int value);
	int colormodel_supported(int colormodel);
	int read_frame_header(const char *path);
	int read_frame(VFrame *output, VFrame *input);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);

	enum
	{
		NONE,
// Data types
		RGB_888,
		RGB_161616,
		RGBA_8888,
		RGBA_16161616,
		RGB_FLOAT,
		RGBA_FLOAT,
		GREYSCALE,
		BLACKWHITE,
// Compression types
		LZW,
		PACK_BITS,
		DEFLATE,
		JPEG
	};

	int tiff_cmodel;

	Mutex *unit_lock;
	static const struct selection_int tiff_compression[];
	static const struct selection_int tiff_cmodels[];
	static struct paramlist_defaults encoder_params[];
};

// For each write frame call, since multiple write_frames are running concurrently.
class FileTIFFUnit : public FrameWriterUnit
{
public:
	FileTIFFUnit(FileTIFF *file, FrameWriter *writer);
	~FileTIFFUnit();

	int offset;
	VFrame *data;
	VFrame *temp;
	FileTIFF *file;
};

#endif
