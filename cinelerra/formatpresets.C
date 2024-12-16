// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bctitle.h"
#include "colormodels.h"
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
	{ "PAL", 2, 2, 48000, 1, 1, 25, 720, 576, 4, 3, BC_ILACE_MODE_BOTTOM_FIRST },
	{ "NTSC", 2, 2, 48000, 1, 1, 30000.0 / 1001, 720, 480, 4, 3, BC_ILACE_MODE_BOTTOM_FIRST },
	{ "PAL Half", 2, 2, 48000, 1, 1, 25, 360, 288, 4, 3, BC_ILACE_MODE_NOTINTERLACED },
	{ "NTSC Half", 2, 2, 48000, 1, 1, 30000.0 / 1001, 360, 240, 4, 3, BC_ILACE_MODE_NOTINTERLACED },
	{ "NTSC Progressive", 2, 2, 48000, 1, 1, 60000.0 / 1001, 720, 480, 4, 3, BC_ILACE_MODE_NOTINTERLACED },
	{ "PAL Progressive", 2, 2, 48000, 1, 1, 25, 720, 576, 4, 3, BC_ILACE_MODE_NOTINTERLACED },
	{ "1080P/60", 2, 2, 48000, 1, 1, 60000.0 / 1001, 1920, 1080, 16, 9, BC_ILACE_MODE_NOTINTERLACED },
	{ "1080P/24", 2, 2, 48000, 1, 1, 24, 1920, 1080, 16, 9, BC_ILACE_MODE_NOTINTERLACED },
	{ "1080I", 2, 2, 48000, 1, 1, 30000.0 / 1001, 1920, 1080, 16, 9, BC_ILACE_MODE_BOTTOM_FIRST },
	{ "720P/60", 2, 2, 48000, 1, 1, 60000.0 / 1001, 1280, 720, 16, 9, BC_ILACE_MODE_NOTINTERLACED },
	{ "Internet", 2, 2, 22050, 1, 1, 15, 320, 240, 4, 3, BC_ILACE_MODE_NOTINTERLACED },
	{ "CD Audio", 2, 2, 44100, 1, 0, 30000.0 / 1001, 720, 480, 4, 3, BC_ILACE_MODE_NOTINTERLACED },
	{ "DAT Audio", 2, 2, 48000, 1, 0, 30000.0 / 1001, 720, 480, 4, 3, BC_ILACE_MODE_NOTINTERLACED },
	{ 0 }
};

#define MAX_NUM_PRESETS ((sizeof(format_presets) / sizeof(struct formatpresets)) - 1)

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
	presets_menu = new selection_int[MAX_NUM_PRESETS + 2];
	presets_menu[0].text = N_("User defined");
	presets_menu[0].value = 0;

	for(int i = 0; i < MAX_NUM_PRESETS; i++)
	{
		presets_menu[i + 1].text = format_presets[i].name;
		presets_menu[i + 1].value = i;
	}
	presets_menu[MAX_NUM_PRESETS + 1].text = 0;
	presets_menu[MAX_NUM_PRESETS + 1].value = 0,
	base_gui->add_subwindow(new BC_Title(x, y, _("Presets:")));
	base_gui->add_subwindow(selection = new FormatSelection(x, y + 20, base_gui,
		presets_menu, this));
}

FormatPresets::~FormatPresets()
{
	delete [] presets_menu;
}

struct formatpresets *FormatPresets::find_preset(EDLSession *edlsession)
{
	for(int i = 0; i < MAX_NUM_PRESETS; i++)
	{
		struct formatpresets *preset = &format_presets[i];
		double ratio = (double)preset->aspect_w / preset->aspect_h *
			preset->output_h / preset->output_w;

		if(edlsession->audio_tracks == preset->audio_tracks &&
			edlsession->audio_channels == preset->audio_channels &&
			edlsession->sample_rate == preset->sample_rate &&
			edlsession->video_tracks == preset->video_tracks &&
			EQUIV(edlsession->frame_rate, preset->frame_rate) &&
			edlsession->output_w == preset->output_w &&
			edlsession->output_h == preset->output_h &&
			EQUIV(edlsession->sample_aspect_ratio, ratio) &&
			edlsession->interlace_mode == preset->interlace_mode)
		{
			return preset;
		}
	}
	return 0;
}

const char* FormatPresets::get_preset_text(EDLSession *edlsession)
{
	struct formatpresets *item = find_preset(edlsession);
	if(item)
		return item->name;
	else
		return "User Defined";
}

void FormatPresets::set_edlsession(EDLSession *edlsession)
{
	selection->update(get_preset_text(edlsession));
	current_edlsession = edlsession;
}

void FormatPresets::update_edlsession(const char *preset)
{
	fill_preset_defaults(preset, current_edlsession);
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
			session->video_tracks = fpr->video_tracks;
			session->frame_rate = fpr->frame_rate;
			session->output_w = fpr->output_w;
			session->output_h = fpr->output_h;
			session->sample_aspect_ratio = (double)fpr->aspect_w / fpr->aspect_h *
				fpr->output_h / fpr->output_w;
			session->interlace_mode = fpr->interlace_mode;
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
	presets->update_edlsession(get_text());
	return 1;
}


IlaceSelection::IlaceSelection(int x, int y,
	BC_WindowBase *base_gui, int *value, struct selection_int *menu)
 : Selection(x, y, base_gui, menu, value, SELECTION_VARWIDTH)
{
	disable(1);
}

void IlaceSelection::update(int value)
{
	BC_TextBox::update(_(AInterlaceModeSelection::name(value)));
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
	BC_TextBox::update(_(name(value)));
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
