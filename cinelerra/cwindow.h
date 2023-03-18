// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CWINDOW_H
#define CWINDOW_H

#include "auto.inc"
#include "autos.inc"
#include "automation.h"
#include "cplayback.inc"
#include "ctracking.inc"
#include "cwindowgui.inc"
#include "datatype.h"
#include "floatauto.inc"
#include "guidelines.inc"
#include "mwindow.inc"
#include "thread.h"
#include "track.inc"

class CWindow : public Thread
{
public:
	CWindow();
	~CWindow();

// Position is inclusive of the other 2
	void update(int options);
	void run();
	Track* calculate_affected_track();
// Get keyframe for editing in the CWindow.
// create - if 0 forces automatic creation to be off
//          if 1 uses automatic creation option to create
	Auto* calculate_affected_auto(int autoidx, ptstime pts, Track *track,
		int create = 1,
		int *created = 0,
		int redraw = 1);
// Same as before.  Provide 0 to Auto arguments to have them ignored.
	void calculate_affected_autos(FloatAuto **x_auto,
		FloatAuto **y_auto,
		FloatAuto **z_auto,
		Track *track,
		int use_camera,
		int create_x,
		int create_y,
		int create_z);
	void show_window();
	void hide_window();
	GuideFrame *new_guideframe(ptstime start, ptstime end, GuideFrame **gframe);
	void delete_guideframe(GuideFrame **gframe);
	int stop_playback();

	int destination;
	CWindowGUI *gui;

	CTracking *playback_cursor;
	CPlayback *playback_engine;
};

#endif
