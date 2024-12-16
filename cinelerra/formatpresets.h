// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FORMATPRESETS_H
#define FORMATPRESETS_H


#include "edlsession.inc"
#include "formatpresets.inc"
#include "mwindow.inc"
#include "new.inc"
#include "setformat.inc"
#include "selection.h"

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
};


class FormatSelection : public Selection
{
public:
	FormatSelection(int x, int y, BC_WindowBase *base_gui, 
		selection_int *menu, FormatPresets *presets);

	int handle_event();
private:
	FormatPresets *presets;
};


class FormatPresets
{
public:
	FormatPresets(BC_WindowBase* base_gui, int x, int y);
	virtual ~FormatPresets();

// Find the item which corresponds to the values in the edlsesion
	struct formatpresets *find_preset(EDLSession *edlsesion);
	const char* get_preset_text(EDLSession *session);

	static void fill_preset_defaults(const char *preset, EDLSession *session);
	void set_edlsession(EDLSession *edlsession);
	void update_edlsession(const char *preset);
// New preset selected
	virtual int handle_event() { return 0; };
private:
	struct selection_int *presets_menu;
	int selection_value;
	FormatSelection *selection;
	EDLSession *current_edlsession;

	static struct formatpresets format_presets[];
};


class IlaceSelection : public Selection
{
public:
	IlaceSelection(int x, int y, BC_WindowBase *base_gui, int *value,
		struct selection_int *menu);

	void update(int value);
};

class AInterlaceModeSelection : public IlaceSelection
{
public:
	AInterlaceModeSelection(int x, int y,
		BC_WindowBase *base_gui, int *value);

	static const char *xml_text(int mode);
	static int xml_value(const char *text);
	static const char *name(int value);
private:
	static struct selection_int ilacemode_selection[];
	static struct selection_int ilacemode_selection_xml[];
};

class InterlaceModeSelection : public IlaceSelection
{
public:
	InterlaceModeSelection(int x, int y,
		BC_WindowBase *base_gui, int *value);
private:
	static struct selection_int ilacemode_selection[];
};

class InterlaceFixSelection : public Selection
{
public:
	InterlaceFixSelection(int x, int y,
		BC_WindowBase *base_gui, int *value);

	void update(int value);

	static const char *name(int value);
	static const char *xml_text(int mode);
	static int xml_value(const char *text);
	static int automode(int projectmode, int assetmode);
	static int automode2(int projectmode, int assetautofixoption,
		int assetmode, int assetfixmethod);

private:
	static struct selection_int ilacefix_selection[];
	static struct selection_int ilacefix_selection_xml[];
};
#endif
