// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VPATCHGUI_H
#define VPATCHGUI_H

#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bcslider.h"
#include "floatauto.inc"
#include "patchgui.h"
#include "vpatchgui.inc"
#include "vtrack.inc"

class VFadePatch;
class VModePatch;

class VPatchGUI : public PatchGUI
{
public:
	VPatchGUI(PatchBay *patchbay, VTrack *track, int x, int y);
	~VPatchGUI();

	int reposition(int x, int y);
	int update(int x, int y);

	VTrack *vtrack;
	VModePatch *mode;
	VFadePatch *fade;
};

class VFadePatch : public BC_ISlider
{
public:
	VFadePatch(VPatchGUI *patch, int x, int y, int w);

	int handle_event();
	double update_edl();
	static double get_keyframe_value(VPatchGUI *patch);

	VPatchGUI *patch;
};

class VModePatch : public BC_PopupMenu
{
public:
	VModePatch(VPatchGUI *patch, int x, int y);

	int handle_event();
	static int get_keyframe_value(VPatchGUI *patch);
	void update(int mode);

	VPatchGUI *patch;
	int mode;
};

class VModePatchItem : public BC_MenuItem
{
public:
	VModePatchItem(VModePatch *popup, const char *text, int mode);

	int handle_event();

	VModePatch *popup;
	int mode;
};

#endif
