
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
#include "selection.h"

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

struct selection_int ColormodelSelection::cmodel_selection[] =
{
	{ 0, BC_RGB888 },
	{ 0, BC_RGBA8888 },
	{ 0, BC_RGB_FLOAT },
	{ 0, BC_RGBA_FLOAT },
	{ 0, BC_YUV888 },
	{ 0, BC_YUVA8888 },
	{ 0, 0 }
};

#define NUM_CMODEL_SELECTIONS (sizeof(cmodel_selection) / sizeof(struct selection_int) - 1)

struct selection_int AInterlaceModeSelection::ilacemode_selection[] =
{
	{ 0, BC_ILACE_MODE_UNDETECTED },
	{ 0, BC_ILACE_MODE_TOP_FIRST },
	{ 0, BC_ILACE_MODE_BOTTOM_FIRST },
	{ 0, BC_ILACE_MODE_NOTINTERLACED },
	{ 0, 0 }
};

struct selection_int InterlaceModeSelection::ilacemode_selection[] =
{
	{ 0, BC_ILACE_MODE_TOP_FIRST },
	{ 0, BC_ILACE_MODE_BOTTOM_FIRST },
	{ 0, BC_ILACE_MODE_NOTINTERLACED },
	{ 0, 0 }
};

#define NUM_ILACEMODE_SELECTIONS (sizeof(ilacemode_selection) / sizeof(struct selection_int) - 1)

FormatPresets::FormatPresets(BC_WindowBase* base_gui, int x, int y)
{
	gui_base = base_gui;
	presets_menu = new selection_int[MAX_NUM_PRESETS + 1];
	presets_menu[0].text = "User defined";
	presets_menu[0].value = 0;

	for(int i = 0; i < MAX_NUM_PRESETS + 1; i++)
	{
		presets_menu[i + 1].text = format_presets[i].name;
		presets_menu[i + 1].value = i;
	}
	gui_base->add_subwindow(new BC_Title(x, y, _("Presets:")));
	gui_base->add_subwindow(selection = new FormatSelection(x, y + 20, gui_base,
		presets_menu, this));
}

FormatPresets::~FormatPresets()
{
	delete [] presets_menu;
}

struct formatpresets *FormatPresets::find_preset(EDL *edl)
{
	for(int i = 0; i < MAX_NUM_PRESETS; i++)
	{
		struct formatpresets *preset = &format_presets[i];

		if(edl->session->audio_tracks == preset->audio_tracks &&
			edl->session->audio_channels == preset->audio_channels &&
			edl->session->sample_rate == preset->sample_rate &&
			edl->session->video_tracks == preset->video_tracks &&
			EQUIV(edl->session->frame_rate, preset->frame_rate) &&
			edl->session->output_w == preset->output_w &&
			edl->session->output_h == preset->output_h &&
			EQUIV(edl->session->aspect_w, preset->aspect_w) &&
			EQUIV(edl->session->aspect_h, preset->aspect_h) &&
			edl->session->interlace_mode == preset->interlace_mode &&
			edl->session->color_model == preset->color_model)
		{
			return preset;
		}
	}
	return 0;
}

const char* FormatPresets::get_preset_text(EDL *edl)
{
	struct formatpresets *item = find_preset(edl);
	if(item)
		return item->name;
	else
		return "User Defined";
}

void FormatPresets::set_edl(EDL *edl)
{
	current_edl = edl;
	selection->update(get_preset_text(edl));
}

void FormatPresets::update_edl(const char *preset)
{
	fill_preset_defaults(preset, current_edl->session);
	handle_event();
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


FormatSelection::FormatSelection(int x, int y, 
	BC_WindowBase *base_gui, selection_int *menu, FormatPresets *presets)
 : Selection(x, y, base_gui, menu, 0, SELECTION_VARWIDTH)
{
	this->presets = presets;
	disable();
}

int FormatSelection::handle_event()
{
	presets->update_edl(get_text());
}


ColormodelSelection::ColormodelSelection(int x, int y,
	BC_WindowBase *base_gui, int *cmodel)
{
	if(!cmodel_selection[0].text)
	{
		for(int i = 0; i < NUM_CMODEL_SELECTIONS; i++)
			cmodel_selection[i].text = cmodel_name(cmodel_selection[i].value);
	}
	base_gui->add_subwindow(selection = new Selection(x, y, base_gui,
		cmodel_selection, cmodel, SELECTION_VARWIDTH));
	selection->disable();
}

void ColormodelSelection::update(int value)
{
	selection->update(cmodel_name(value));
}


IlaceSelection::IlaceSelection(int x, int y,
	BC_WindowBase *base_gui, int *value,
	struct selection_int *menu, int menulen)
{
	if(!menu[0].text)
	{
		for(int i = 0; i < menulen; i++)
			menu[i].text = ilacemode_name(menu[i].value);
	}
	base_gui->add_subwindow(selection = new Selection(x, y, base_gui,
		menu, value, SELECTION_VARWIDTH));
	selection->disable();
}

void IlaceSelection::update(int value)
{
	selection->update(ilacemode_name(value));
}

AInterlaceModeSelection::AInterlaceModeSelection(int x, int y,
	BC_WindowBase *base_gui, int *value)
 : IlaceSelection(x, y, base_gui, value, ilacemode_selection, NUM_ILACEMODE_SELECTIONS)
{
}

InterlaceModeSelection::InterlaceModeSelection(int x, int y,
	BC_WindowBase *base_gui, int *value)
 : IlaceSelection(x, y, base_gui, value, ilacemode_selection, NUM_ILACEMODE_SELECTIONS)
{
}
