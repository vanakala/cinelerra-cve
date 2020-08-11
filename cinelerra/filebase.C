// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.inc"
#include "bctitle.h"
#include "file.h"
#include "filebase.h"
#include "language.h"
#include "mwindow.h"

#include <stdlib.h>


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

FBConfig::FBConfig(BC_WindowBase *parent_window, int type, int absx, int absy)
 : BC_Window(MWindow::create_title(N_("Compression options")),
	absx,
	absy,
	350,
	100)
{
	set_icon(mwindow_global->get_window_icon());
	if(type & SUPPORTS_AUDIO)
		add_tool(new BC_Title(10, 10, _("There are no audio options for this format")));
	else
		add_tool(new BC_Title(10, 10, _("There are no video options for this format")));
	add_subwindow(new BC_OKButton(this));
}
