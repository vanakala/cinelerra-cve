// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bctitle.h"
#include "bclistboxitem.h"
#include "bcsignals.h"
#include "fileformat.h"
#include "language.h"
#include "mwindow.h"
#include "selection.h"
#include "theme.h"
#include "new.h"

extern "C"
{
#include <libavformat/avformat.h>
}

// Names copied from libavformat/pcmdec.c
struct format_names
{
	const char *name;
	const char *longname;
	int bytes_sample;
}

FileFormatPCMFormat::pcm_formats[] =
{
	{ "f64be", 0, 8 },
	{ "f64le", 0, 8 },
	{ "f32be", 0, 4 },
	{ "f32le", 0, 4 },
	{ "s32le", 0, 4 },
	{ "s24be", 0, 3 },
	{ "s24le", 0, 3 },
	{ "s16be", 0, 2 },
	{ "s16le", 0, 2 },
	{ "s8", 0, 1 },
	{ "u32be", 0, 4 },
	{ "u32le", 0, 4 },
	{ "u24be", 0, 3 },
	{ "u24le", 0, 3 },
	{ "u16be", 0, 2 },
	{ "u16le", 0, 2 },
	{ "u8", 0, 1 },
	{ "alaw", 0, 1 },
	{ "mulaw", 0, 1 },
	{ "sln", 0, 2 },
	{ 0, 0 }
};

FileFormat::FileFormat(Asset *asset, const char *string2, int absx, int absy)
 : BC_Window(MWindow::create_title(N_("File Format")),
	absx, absy, 500, 220, 500, 220)
{
	int x = 10;
	int y = 10;
	BC_Title *title;
	int text_w = 0;
	int y1, y2, y3, w;

	sdsc = &asset->streams[0];
	this->asset = asset;

	set_icon(mwindow_global->get_window_icon());

	add_subwindow(title = new BC_Title(x, y, string2));
	y += title->get_h() + 8;
	add_subwindow(title = new BC_Title(x, y, _("Assuming raw PCM:")));

	y1 = y += title->get_h() + 8;
	add_subwindow(title = new BC_Title(x, y, _("Channels:")));

	text_w = title->get_w();
	y2 = y += title->get_h() + 8;
	add_subwindow(title = new BC_Title(x, y, _("Sample rate:")));

	if(text_w < (w = title->get_w()))
		text_w = w;
	y3 = y += title->get_h() + 8;
	add_subwindow(title = new BC_Title(x, y, _("Sample format:")));

	if(text_w < (w = title->get_w()))
		text_w = w;

	x += text_w + 10;
	channels_button = new FileFormatChannels(x, y1, this, sdsc->channels);
	add_subwindow(rate_button = new SampleRateSelection(x, y2, this,
		&sdsc->sample_rate));
	pcmformat = new FileFormatPCMFormat(x, y3, this, sdsc->codec);
	rate_button->update(sdsc->sample_rate);

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
}

FileFormat::~FileFormat()
{
	delete rate_button;
	delete channels_button;
	delete pcmformat;
}


FileFormatChannels::FileFormatChannels(int x, int y, FileFormat *fwindow, int value)
 : BC_TumbleTextBox(fwindow, 
	value,
	1,
	MAXCHANNELS,
	x, 
	y, 
	50)
{
	this->fwindow = fwindow;
}

int FileFormatChannels::handle_event()
{
	fwindow->sdsc->channels = atol(get_text());
	return 1;
}


FileFormatPCMFormat::FileFormatPCMFormat(int x, int y, FileFormat *fwindow,
	const char *selected)
 : BC_PopupTextBox(fwindow, 0, 0, x, y, 300, 300)
{
	this->fwindow = fwindow;

	if(!pcm_formats[0].longname)
	{
		AVInputFormat *ifmt;

		for(int i = 0; pcm_formats[i].name; i++)
		{
			ifmt = av_find_input_format(pcm_formats[i].name);
			pcm_formats[i].longname = ifmt->long_name;
		}
	}

	for(int i = 0; pcm_formats[i].name; i++)
	{
		if(!pcm_formats[i].longname)
			continue;
		if(selected && !strcmp(pcm_formats[i].name, selected))
		{
			update(pcm_formats[i].longname);
			fwindow->sdsc->bytes = pcm_formats[i].bytes_sample;
		}
		formats.append(new BC_ListBoxItem(pcm_formats[i].longname));
	}
	update_list(&formats);
}

int FileFormatPCMFormat::handle_event()
{
	int num = get_number();

	for(int i = 0; pcm_formats[i].name; i++)
	{
		if(!pcm_formats[i].longname)
			continue;
		if(i == num)
		{
			strcpy(fwindow->sdsc->codec, pcm_formats[i].name);
			fwindow->sdsc->bytes = pcm_formats[i].bytes_sample;
		}
	}
	return 1;
}

const char *FileFormatPCMFormat::pcm_format(const char *fmt_txt)
{
	for(int i = 0; pcm_formats[i].name; i++)
	{
		if(strcmp(pcm_formats[i].name, fmt_txt) == 0)
			return pcm_formats[i].name;
	}
	return 0;
}
