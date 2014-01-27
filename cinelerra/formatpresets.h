
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

#ifndef FORMATPRESETS_H
#define FORMATPRESETS_H


#include "edl.inc"
#include "formatpresets.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "new.inc"
#include "setformat.inc"

struct formatpresets
{
	const char *name;
	int audio_channels;
	int audio_tracks;
	int sample_rate;
	int video_channels;
	int video_tracks;
	double frame_rate;
	int output_w;
	int output_h;
	int aspect_w;
	int aspect_h;
	int interlace_mode;
	int color_model;
};


class FormatPresets
{
public:
	FormatPresets(MWindow *mwindow,
		NewWindow *new_gui, 
		SetFormatWindow *format_gui, 
		int x, 
		int y);
	virtual ~FormatPresets();

	void create_objects();
// Find the listbox item which corresponds to the values in the edl.
	FormatPresetItem* find_preset(EDL *edl);
	const char* get_preset_text(EDL *edl);

	static void fill_preset_defaults(const char *preset, EDLSession *session);

// New preset selected
	virtual int handle_event() { return 0; };
	virtual EDL* get_edl() { return 0; };

	MWindow *mwindow;
	BC_WindowBase *gui_base;
	NewWindow *new_gui;
	SetFormatWindow *format_gui;
	FormatPresetsText *text;
	FormatPresetsPulldown *pulldown;
	int x, y;
	ArrayList<FormatPresetItem*> preset_items;

	static struct formatpresets format_presets[];
};


class FormatPresetsText : public BC_TextBox
{
public:
	FormatPresetsText(MWindow *mwindow, 
		FormatPresets *gui,
		int x, 
		int y);

	int handle_event() { return 0; };

	FormatPresets *gui;
	MWindow *mwindow;
};


class FormatPresetsPulldown : public BC_ListBox
{
public:
	FormatPresetsPulldown(MWindow *mwindow, 
		FormatPresets *gui, 
		int x, 
		int y);
	int handle_event();
	MWindow *mwindow;
	FormatPresets *gui;
};


class FormatPresetItem : public BC_ListBoxItem
{
public:
	FormatPresetItem(MWindow *mwindow, FormatPresets *gui, const char *text);
	~FormatPresetItem();

	MWindow *mwindow;
	FormatPresets *gui;
// Storage of the values for the preset
	EDL *edl;
};

#endif
