
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

#ifndef FILEPNG_H
#define FILEPNG_H


#include "file.inc"
#include "filebase.h"
#include "filelist.h"
#include "vframe.inc"

class FilePNG : public FileList
{
public:
	FilePNG(Asset *asset, File *file);
	~FilePNG();

	static int check_sig(Asset *asset);
	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);
	static int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	int can_copy_from(Edit *edit, int64_t position);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);

	int read_frame_header(char *path);



	int native_cmodel;
// For decoding only
	VFrame *temp;
};


class PNGUnit : public FrameWriterUnit
{
public:
	PNGUnit(FilePNG *file, FrameWriter *writer);
	~PNGUnit();
	
	FilePNG *file;
	VFrame *temp_frame;
};

class PNGConfigVideo : public BC_Window
{
public:
	PNGConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~PNGConfigVideo();

	int create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	Asset *asset;
};


class PNGUseAlpha : public BC_CheckBox
{
public:
	PNGUseAlpha(PNGConfigVideo *gui, int x, int y);
	int handle_event();
	PNGConfigVideo *gui;
};


#endif
