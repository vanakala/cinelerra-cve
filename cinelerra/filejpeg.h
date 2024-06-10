// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILEJPEG_H
#define FILEJPEG_H

#include "asset.inc"
#include "file.inc"
#include "filelist.h"
#include "vframe.inc"
#include <jpeglib.h>

class FileJPEG : public FileList
{
public:
	FileJPEG(Asset *asset, File *file);
	~FileJPEG();

// basic commands for every file interpreter
	static int check_sig(Asset *asset);
	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int options);

	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	int read_frame_header(const char *path);
	static void save_render_options(Asset *asset);
	static void load_render_options(Asset *asset);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);
	static void dump_jpeg_decompress_struct(jpeg_decompress_struct *jpeg,
		int indent);
	static const char *color_space_name(J_COLOR_SPACE cspce);

private:
	void show_jpeg_error(j_common_ptr cinfo);
	VFrame *temp_frame;
	static struct paramlist_defaults encoder_params[];
};

class JPEGUnit : public FrameWriterUnit
{
public:
	JPEGUnit(FileJPEG *file, FrameWriter *writer);
	~JPEGUnit();

	VFrame *temp_frame;
	unsigned char *compressed;
};

#endif
