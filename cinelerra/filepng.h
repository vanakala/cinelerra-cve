// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILEPNG_H
#define FILEPNG_H

#include "asset.inc"
#include "datatype.h"
#include "file.inc"
#include "filebase.h"
#include "filelist.h"
#include "vframe.inc"

class FilePNG : public FileList
{
public:
	FilePNG(Asset *asset, File *file);

	static int check_sig(Asset *asset);
	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int options);
	int colormodel_supported(int colormodel);
	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);
	static void save_render_optios(Asset *asset);
	static void get_render_defaults(Asset *asset);
	int read_frame_header(const char *path);

private:
	int native_cmodel;
	static struct paramlist_defaults encoder_params[];
};


class PNGUnit : public FrameWriterUnit
{
public:
	PNGUnit(FilePNG *file, FrameWriter *writer);
	~PNGUnit();

	FilePNG *file;
	VFrame *temp_frame;
};

#endif
