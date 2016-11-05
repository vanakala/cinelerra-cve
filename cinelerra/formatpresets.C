
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
	{ N_("Unknown"), BC_ILACE_MODE_UNDETECTED },
	{ N_("Top Fields First"), BC_ILACE_MODE_TOP_FIRST },
	{ N_("Bottom Fields First"), BC_ILACE_MODE_BOTTOM_FIRST },
	{ N_("Not Interlaced"), BC_ILACE_MODE_NOTINTERLACED },
	{ 0, 0 }
};

struct selection_int InterlaceModeSelection::ilacemode_selection[] =
{
	{ N_("Top Fields First"), BC_ILACE_MODE_TOP_FIRST },
	{ N_("Bottom Fields First"), BC_ILACE_MODE_BOTTOM_FIRST },
	{ N_("Not Interlaced"), BC_ILACE_MODE_NOTINTERLACED },
	{ 0, 0 }
};

struct selection_int InterlaceFixSelection::ilacefix_selection[] =
{
	{ N_("Do Nothing"), BC_ILACE_FIXMETHOD_NONE },
	{ N_("Shift Up 1 pixel"), BC_ILACE_FIXMETHOD_UPONE },
	{ N_("Shift Down 1 pixel"), BC_ILACE_FIXMETHOD_DOWNONE },
	{ 0, 0 }
};

struct selection_int AInterlaceModeSelection::ilacemode_selection_xml[] =
{
	{ "UNKNOWN", BC_ILACE_MODE_UNDETECTED },
	{ "TOP_FIELD_FIRST", BC_ILACE_MODE_TOP_FIRST },
	{ "BOTTOM_FIELD_FIRST", BC_ILACE_MODE_BOTTOM_FIRST },
	{ "NOTINTERLACED", BC_ILACE_MODE_NOTINTERLACED },
	{ 0, 0 }
};

struct selection_int InterlaceFixSelection::ilacefix_selection_xml[] =
{
	{ "DO_NOTHING", BC_ILACE_FIXMETHOD_NONE },
	{ "SHIFT_UPONE", BC_ILACE_FIXMETHOD_UPONE },
	{ "SHIFT_DOWNONE", BC_ILACE_FIXMETHOD_DOWNONE },
	{ 0, 0 }
};


FormatPresets::FormatPresets(BC_WindowBase* base_gui, int x, int y)
{
	gui_base = base_gui;
	presets_menu = new selection_int[MAX_NUM_PRESETS + 1];
	presets_menu[0].text = N_("User defined");
	presets_menu[0].value = 0;

	for(int i = 0; i < MAX_NUM_PRESETS; i++)
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
	disable(1);
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
			cmodel_selection[i].text = ColorModels::name(cmodel_selection[i].value);
	}
	base_gui->add_subwindow(selection = new Selection(x, y, base_gui,
		cmodel_selection, cmodel, SELECTION_VARWIDTH));
	selection->disable(1);
}

void ColormodelSelection::update(int value)
{
	selection->update(ColorModels::name(value));
}


IlaceSelection::IlaceSelection(int x, int y,
	BC_WindowBase *base_gui, int *value, struct selection_int *menu)
 : Selection(x, y, base_gui, menu, value, SELECTION_VARWIDTH)
{
	disable(1);
}

void IlaceSelection::update(int value)
{
	BC_TextBox::update(AInterlaceModeSelection::name(value));
}

AInterlaceModeSelection::AInterlaceModeSelection(int x, int y,
	BC_WindowBase *base_gui, int *value)
 : IlaceSelection(x, y, base_gui, value, ilacemode_selection)
{
}

const char *AInterlaceModeSelection::name(int value)
{
	for(int i = 0; ilacemode_selection[i].text; i++)
	{
		if(value == ilacemode_selection[i].value)
			return ilacemode_selection[i].text;
	}
	return ilacemode_selection[0].text;
}

const char *AInterlaceModeSelection::xml_text(int value)
{
	for(int i = 0; ilacemode_selection_xml[i].text; i++)
	{
		if(value == ilacemode_selection_xml[i].value)
			return ilacemode_selection_xml[i].text;
	}
	return ilacemode_selection_xml[0].text;
}

int AInterlaceModeSelection::xml_value(const char *text)
{
	for(int i = 0; ilacemode_selection_xml[i].text; i++)
	{
		if(!strcmp(ilacemode_selection_xml[i].text, text))
			return ilacemode_selection_xml[i].value;
	}
	return BC_ILACE_MODE_UNDETECTED;
}

InterlaceModeSelection::InterlaceModeSelection(int x, int y,
	BC_WindowBase *base_gui, int *value)
 : IlaceSelection(x, y, base_gui, value, ilacemode_selection)
{
}

InterlaceFixSelection::InterlaceFixSelection(int x, int y,
	BC_WindowBase *base_gui, int *value)
 : Selection(x, y, base_gui, ilacefix_selection, value, SELECTION_VARWIDTH)
{
	disable(1);
}

void InterlaceFixSelection::update(int value)
{
	BC_TextBox::update(name(value));
}

const char *InterlaceFixSelection::name(int value)
{
	for(int i = 0; ilacefix_selection[i].text; i++)
	{
		if(value == ilacefix_selection[i].value)
			return ilacefix_selection[i].text;
	}
	return ilacefix_selection[0].text;
}

const char *InterlaceFixSelection::xml_text(int value)
{
	for(int i = 0; ilacefix_selection_xml[i].text; i++)
	{
		if(value == ilacefix_selection_xml[i].value)
			return ilacefix_selection_xml[i].text;
	}
	return ilacefix_selection_xml[0].text;
}

int InterlaceFixSelection::xml_value(const char *text)
{
	for(int i = 0; ilacefix_selection_xml[i].text; i++)
	{
		if(!strcmp(ilacefix_selection_xml[i].text, text))
			return ilacefix_selection_xml[i].value;
	}
	return BC_ILACE_MODE_UNDETECTED;
}

int InterlaceFixSelection::automode(int projectmode, int assetmode)
{
	if(projectmode == assetmode)
		return BC_ILACE_FIXMETHOD_NONE;

	if((projectmode == BC_ILACE_MODE_BOTTOM_FIRST && assetmode == BC_ILACE_MODE_TOP_FIRST) ||
			(projectmode == BC_ILACE_MODE_TOP_FIRST  && assetmode == BC_ILACE_MODE_BOTTOM_FIRST))
		return BC_ILACE_FIXMETHOD_UPONE;

	return BC_ILACE_FIXMETHOD_NONE;
}

int InterlaceFixSelection::automode2(int projectmode, int assetautofixoption,
	int assetmode, int assetfixmethod)
{
	if(assetautofixoption)
		return automode(projectmode, assetmode);
	else
		return assetfixmethod;
}
