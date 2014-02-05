
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
	int color_model;
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

// Find the item which corresponds to the values in the edl.
	struct formatpresets *find_preset(EDL *edl);
	const char* get_preset_text(EDL *edl);

	static void fill_preset_defaults(const char *preset, EDLSession *session);
	void set_edl(EDL *edl);
	void update_edl(const char *preset);
// New preset selected
	virtual int handle_event() { return 0; };
private:
	BC_WindowBase *gui_base;
	struct selection_int *presets_menu;
	int selection_value;
	FormatSelection *selection;
	EDL *current_edl;

	static struct formatpresets format_presets[];
};


class ColormodelSelection
{
public:
	ColormodelSelection(int x, int y, BC_WindowBase *base_gui, int *cmodel);

	void update(int value);

	Selection *selection;
private:
	static struct selection_int cmodel_selection[];
};

class IlaceSelection
{
public:
	IlaceSelection(int x, int y, BC_WindowBase *base_gui, int *value,
		struct selection_int *menu, int menulen);

	void update(int value);

	Selection *selection;
};

class AInterlaceModeSelection : public IlaceSelection
{
public:
	AInterlaceModeSelection(int x, int y,
		BC_WindowBase *base_gui, int *value);
private:
	static struct selection_int ilacemode_selection[];
};

class InterlaceModeSelection : public IlaceSelection
{
public:
	InterlaceModeSelection(int x, int y,
		BC_WindowBase *base_gui, int *value);
private:
	static struct selection_int ilacemode_selection[];
};

class InterlaceFixSelection
{
public:
	InterlaceFixSelection(int x, int y,
		BC_WindowBase *base_gui, int *value);

	void update(int value);

	Selection *selection;
private:
	static struct selection_int ilacefix_selection[];
};
#endif
