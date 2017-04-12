
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

#include "asset.inc"
#include "assets.h"
#include "file.h"
#include "filebase.h"
#include "language.h"
#include "mwindow.h"
#include "theme.h"

#include <stdlib.h>

extern Theme *theme_global;

FileBase::FileBase(Asset *asset, File *file)
{
	this->file = file;
	this->asset = asset;
	dither = 0;
}

FileBase::~FileBase()
{
}

void FileBase::set_dither()
{
	dither = 1;
}

int FileBase::match4(const char *in, const char *out)
{
	if(in[0] == out[0] &&
		in[1] == out[1] &&
		in[2] == out[2] &&
		in[3] == out[3])
		return 1;
	else
		return 0;
}

FBConfig::FBConfig(BC_WindowBase *parent_window, int type)
 : BC_Window(MWindow::create_title(N_("Compression options")),
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	350,
	100)
{
	set_icon(theme_global->get_image("mwindow_icon"));
	if(type & SUPPORTS_AUDIO)
		add_tool(new BC_Title(10, 10, _("There are no audio options for this format")));
	else
		add_tool(new BC_Title(10, 10, _("There are no video options for this format")));
	add_subwindow(new BC_OKButton(this));
}
