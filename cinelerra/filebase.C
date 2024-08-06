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
