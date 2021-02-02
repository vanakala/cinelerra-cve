// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PATCHBAY_H
#define PATCHBAY_H

#include "bcsubwindow.h"
#include "bcpixmap.h"
#include "filexml.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "overlayframe.inc"
#include "patchbay.inc"
#include "patchgui.inc"


class PatchBay : public BC_SubWindow
{
public:
	PatchBay();
	~PatchBay();

	void delete_all_patches();
	void show();
	void resize_event();
	int cursor_motion_event();
	BC_Pixmap* mode_to_icon(int mode);
	int icon_to_mode(BC_Pixmap *icon);
// Get the patch that matches the track.
	PatchGUI* get_patch_of(Track *track);

// Synchronize with Master EDL
	void update();
	void update_meters(double *module_levels, int total);
	void stop_meters();
	void synchronize_nudge(ptstime value, Track *skip);
	void synchronize_faders(double value, int data_type, Track *skip);
	void change_meter_format(int min, int max);
	void reset_meters();
	void set_delays(int over_delay, int peak_delay);

	ArrayList<PatchGUI*> patches;

// =========================================== drawing

	void resize_event(int top, int bottom);
	Track *is_over_track();     // called from trackcanvas!

	int button_down;
	int new_status;
	int drag_operation;
	int reconfigure_trigger;
	BC_Pixmap *mode_icons[TRANSFER_TYPES];
};

#endif
