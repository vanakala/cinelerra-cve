// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef APATCHGUI_H
#define APATCHGUI_H

#include "apatchgui.inc"
#include "atrack.inc"
#include "bcslider.h"
#include "bcpan.h"
#include "bcmeter.h"
#include "bcwindow.h"
#include "floatauto.inc"
#include "panauto.inc"
#include "patchgui.h"

class AFadePatch;
class APanPatch;
class AMeterPatch;

class APatchGUI : public PatchGUI
{
public:
	APatchGUI(MWindow *mwindow, PatchBay *patchbay, ATrack *track, int x, int y);
	~APatchGUI();

	int reposition(int x, int y);
	int update(int x, int y);
	void synchronize_fade(float value_change);

	ATrack *atrack;
	AFadePatch *fade;
	APanPatch *pan;
	AMeterPatch *meter;
};

class AFadePatch : public BC_FSlider
{
public:
	AFadePatch(MWindow *mwindow, APatchGUI *patch, int x, int y, int w);
	static float get_keyframe_value(MWindow *mwindow, APatchGUI *patch);
	int handle_event();
	float update_edl();
	MWindow *mwindow;
	APatchGUI *patch;
};

class APanPatch : public BC_Pan
{
public:
	APanPatch(MWindow *mwindow, APatchGUI *patch, int x, int y,
		int handle_x, int handle_y, double *values);

	int handle_event();
	MWindow *mwindow;
	APatchGUI *patch;
};

class AMeterPatch : public BC_Meter
{
public:
	AMeterPatch(MWindow *mwindow, APatchGUI *patch, int x, int y);
	int button_press_event();
	MWindow *mwindow;
	APatchGUI *patch;
};


#endif
