
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

#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "formatpresets.h"
#include "interlacemodes.h"
#include "language.h"
#include "mwindow.h"

struct formatpresets FormatPresets::format_presets[] =
{
	{ "PAL", 2, 2, 48000, 1, 1, 25, 720, 576, 4, 3, BC_ILACE_MODE_BOTTOM_FIRST, BC_YUVA8888 },
	{ "NTSC", 2, 2, 48000, 1, 1, 30000.0 / 1001, 720, 480, 4, 3, BC_ILACE_MODE_BOTTOM_FIRST, BC_YUVA8888 },
	{ "PAL Half", 2, 2, 48000, 1, 1, 25, 360, 288, 4, 3, BC_ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ "NTSC Half", 2, 2, 48000, 1, 1, 30000.0 / 1001, 360, 240, 4, 3, BC_ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ "NTSC Progressive", 2, 2, 48000, 1, 1, 60000.0 / 1001, 720, 480, 4, 3, BC_ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ "PAL Progressive", 2, 2, 48000, 1, 1, 25, 720, 576, 4, 3, BC_ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ "1080P/60", 2, 2, 48000, 1, 1, 60000.0 / 1001, 1920, 1080, 16, 9, BC_ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ "1080P/24", 2, 2, 48000, 1, 1, 24, 1920, 1080, 16, 9, BC_ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ "1080I", 2, 2, 48000, 1, 1, 30000.0 / 1001, 1920, 1080, 16, 9, BC_ILACE_MODE_BOTTOM_FIRST, BC_YUVA8888 },
	{ "720P/60", 2, 2, 48000, 1, 1, 60000.0 / 1001, 1280, 720, 16, 9, BC_ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ "Internet", 2, 2, 22050, 1, 1, 15, 320, 240, 4, 3, BC_ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ "CD Audio", 2, 2, 44100, 1, 0, 30000.0 / 1001, 720, 480, 4, 3, BC_ILACE_MODE_NOTINTERLACED, BC_RGBA8888 },
	{ "DAT Audio", 2, 2, 48000, 1, 0, 30000.0 / 1001, 720, 480, 4, 3, BC_ILACE_MODE_NOTINTERLACED, BC_RGBA8888 },
	{ 0 }
};

#define MAX_NUM_PRESETS (sizeof(format_presets) / sizeof(struct formatpresets))

FormatPresets::FormatPresets(MWindow *mwindow,
	NewWindow *new_gui, 
	SetFormatWindow *format_gui, 
	int x, 
	int y)
{
	this->mwindow = mwindow;
	this->new_gui = new_gui;
	this->format_gui = format_gui;
	gui_base = new_gui ? (BC_WindowBase*)new_gui : (BC_WindowBase*)(format_gui);
	this->x = x;
	this->y = y;
	text = 0;
	pulldown = 0;
}

FormatPresets::~FormatPresets()
{
	if(text) delete text;
	if(pulldown) delete pulldown;
	for(int i = 0; i < preset_items.total; i++)
		delete preset_items.values[i];
}

void FormatPresets::create_objects()
{
// Create preset items
	FormatPresetItem *item;
	int i;
	const char *p;
	item = new FormatPresetItem(mwindow, this, _("User Defined"));
	preset_items.append(item);

	for(i = 0; p = format_presets[i].name; i++)
	{
		item = new FormatPresetItem(mwindow, this, p);
		fill_preset_defaults(p, item->edl->session);
		preset_items.append(item);
	}

	gui_base->add_subwindow(new BC_Title(x, y, _("Presets:")));
	int x1 = x;
	y += 20;

	gui_base->add_subwindow(text = new FormatPresetsText(mwindow, 
		this, 
		x, 
		y));
	x += text->get_w();
	gui_base->add_subwindow(pulldown = new FormatPresetsPulldown(mwindow, 
		this, 
		x, 
		y));
	x = x1;
}

FormatPresetItem* FormatPresets::find_preset(EDL *edl)
{
	for(int i = 1; i < preset_items.total; i++)
	{
		FormatPresetItem *preset = preset_items.values[i];
		if(edl->session->audio_tracks == preset->edl->session->audio_tracks &&
			edl->session->audio_channels == preset->edl->session->audio_channels &&
			edl->session->sample_rate == preset->edl->session->sample_rate &&
			edl->session->video_tracks == preset->edl->session->video_tracks &&
			EQUIV(edl->session->frame_rate, preset->edl->session->frame_rate) &&
			edl->session->output_w == preset->edl->session->output_w &&
			edl->session->output_h == preset->edl->session->output_h &&
			EQUIV(edl->session->aspect_w, preset->edl->session->aspect_w) &&
			EQUIV(edl->session->aspect_h, preset->edl->session->aspect_h) &&
			edl->session->interlace_mode == preset->edl->session->interlace_mode &&
			edl->session->color_model == preset->edl->session->color_model)
		{
			return preset;
		}
	}
	return 0;
}

const char* FormatPresets::get_preset_text(EDL *edl)
{
	FormatPresetItem *item = find_preset(edl);
	if(item) 
		return item->get_text();
	else
		return "User Defined";
}

void FormatPresets::fill_preset_defaults(const char *preset, EDLSession *session)
{
	struct formatpresets *fpr;

	for(fpr = &format_presets[0]; fpr->name; fpr++)
	{
		if(strcmp(fpr->name, preset) == 0)
		{
			session->audio_channels = fpr->audio_channels;
			session->audio_tracks = fpr->audio_tracks;
			session->sample_rate = fpr->sample_rate;
			session->video_channels = fpr->video_channels;
			session->video_tracks = fpr->video_tracks;
			session->frame_rate = fpr->frame_rate;
			session->output_w = fpr->output_w;
			session->output_h = fpr->output_h;
			session->aspect_w = fpr->aspect_w;
			session->aspect_h = fpr->aspect_h;
			session->interlace_mode = fpr->interlace_mode;
			session->color_model = fpr->color_model;
			return;
		}
	}
}

FormatPresetsText::FormatPresetsText(MWindow *mwindow, 
	FormatPresets *gui,
	int x, 
	int y)
 : BC_TextBox(x, 
	y,
	200, 
	1, 
	gui->get_preset_text(gui->get_edl()))
{
	this->gui =  gui;
	this->mwindow = mwindow;
}


FormatPresetsPulldown::FormatPresetsPulldown(MWindow *mwindow, 
	FormatPresets *gui, 
	int x, 
	int y)
 : BC_ListBox(x, 
	y, 
	200, 
	250,
	(ArrayList<BC_ListBoxItem*>*)&gui->preset_items, // Each column has an ArrayList of BC_ListBoxItems.
	LISTBOX_POPUP)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int FormatPresetsPulldown::handle_event()
{
	gui->handle_event();
	FormatPresetItem *preset = ((FormatPresetItem*)get_selection(0, 0));
	gui->get_edl()->copy_all(preset->edl);
	gui->text->update(preset->get_text());
	gui->handle_event();
	return 1;
}

FormatPresetItem::FormatPresetItem(MWindow *mwindow, 
	FormatPresets *gui, 
	const char *text)
 : BC_ListBoxItem(text)
{
	this->mwindow = mwindow;
	this->gui = gui;
	edl = new EDL;
	edl->copy_all(gui->get_edl());
}

FormatPresetItem::~FormatPresetItem()
{
	delete edl;
}

