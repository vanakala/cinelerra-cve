// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILEFORMAT_H
#define FILEFORMAT_H

class FileFormatChannels;
class FileFormatPCMFormat;
struct format_names;

#include "asset.inc"
#include "bctextbox.h"
#include "bctoggle.h"
#include "bcwindow.h"
#include "fileformat.inc"
#include "selection.inc"

class FileFormat : public BC_Window
{
public:
	FileFormat(Asset *asset, const char *string2, int absx, int absy);
	~FileFormat();

	Asset *asset;
	struct streamdesc *sdsc;
	Selection *rate_button;
	FileFormatChannels *channels_button;
	FileFormatPCMFormat *pcmformat;
};


class FileFormatChannels : public BC_TumbleTextBox
{
public:
	FileFormatChannels(int x, int y, FileFormat *fwindow, int value);

	int handle_event();

	FileFormat *fwindow;
};


class FileFormatPCMFormat : public BC_PopupTextBox
{
public:
	FileFormatPCMFormat(int x, int y, FileFormat *fwindow,
		const char *selected);

	int handle_event();
	static const char *pcm_format(const char *fmt_txt);

private:
	FileFormat *fwindow;
	static struct format_names pcm_formats[];
	ArrayList<BC_ListBoxItem*> formats;
};

#endif
