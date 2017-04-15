
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
	PatchBay(MWindow *mwindow, MWindowGUI *gui);
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
	void synchronize_nudge(posnum value, Track *skip);
	void synchronize_faders(float value, int data_type, Track *skip);
	void change_meter_format(int min, int max);
	void reset_meters();
	void set_delays(int over_delay, int peak_delay);

	ArrayList<PatchGUI*> patches;

// =========================================== drawing

	void resize_event(int top, int bottom);
	Track *is_over_track();	// called from trackcanvas!

	MWindow *mwindow;
	MWindowGUI *gui;

	int button_down, new_status, drag_operation, reconfigure_trigger;
	BC_Pixmap *mode_icons[TRANSFER_TYPES];
};

#endif
