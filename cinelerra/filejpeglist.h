
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

#ifndef FILEJPEGLIST_H
#define FILEJPEGLIST_H

#include "file.inc"
#include "filebase.h"
#include "filelist.h"
#include "jpegwrapper.h"
#include "vframe.inc"

class FileJPEGList : public FileList
{
public:
	FileJPEGList(Asset *asset, File *file);
	~FileJPEGList();


	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);

	int get_best_colormodel(int driver, int colormodel);
	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data);

	char* list_title();
	char* extension();

// Direct copy routines
	int can_copy_from(Asset *asset);
};


class JPEGConfigVideo : public BC_Window
{
public:
	JPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~JPEGConfigVideo();

	int create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	Asset *asset;
};

#endif
