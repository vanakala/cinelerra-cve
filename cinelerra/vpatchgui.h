
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

#ifndef VPATCHGUI_H
#define VPATCHGUI_H

#include "floatauto.inc"
#include "guicast.h"
#include "patchgui.h"
#include "vpatchgui.inc"
#include "vtrack.inc"

class VFadePatch;
class VModePatch;

class VPatchGUI : public PatchGUI
{
public:
	VPatchGUI(MWindow *mwindow, 
		PatchBay *patchbay, 
		VTrack *track, 
		int x, 
		int y);
	~VPatchGUI();

	int create_objects();
	int reposition(int x, int y);
	int update(int x, int y);
	void synchronize_fade(float value_change);

	VTrack *vtrack;
	VModePatch *mode;
	VFadePatch *fade;
};

class VFadePatch : public BC_ISlider
{
public:
	VFadePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y, int w);
	int handle_event();
	float update_edl();
	static FloatAuto* get_keyframe(MWindow *mwindow, VPatchGUI *patch);
	MWindow *mwindow;
	VPatchGUI *patch;
};

class VModePatch : public BC_PopupMenu
{
public:
	VModePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y);

	int handle_event();
	int create_objects();         // add initial items
	static char* mode_to_text(int mode);
	static IntAuto* get_keyframe(MWindow *mwindow, VPatchGUI *patch);
	void update(int mode);

	MWindow *mwindow;
	VPatchGUI *patch;
	int mode;
};

class VModePatchItem : public BC_MenuItem
{
public:
	VModePatchItem(VModePatch *popup, char *text, int mode);

	int handle_event();
	VModePatch *popup;
	int mode;
};


#endif
