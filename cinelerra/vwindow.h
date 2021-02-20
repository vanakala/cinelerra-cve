// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VWINDOW_H
#define VWINDOW_H

#include "asset.inc"
#include "edl.inc"
#include "edlsession.inc"
#include "thread.h"
#include "vframe.inc"
#include "vplayback.inc"
#include "vtracking.inc"
#include "vwindowgui.inc"

class VWindow : public Thread
{
public:
	VWindow();
	~VWindow();

	void run();
// Change source to asset, creating a new EDL
	void change_source(Asset *asset);
// Change source to EDL
	void change_source(EDL *edl);
// Change source to master EDL's vwindow EDL after a load.
	void change_source();
// Remove source
	void remove_source(Asset *asset = 0);
	void update_position(int use_slider = 1,
		int update_slider = 0);
	void set_inpoint();
	void set_outpoint();
	void copy();
	void goto_start();
	void goto_end();
	void update(int options);
	int stop_playback();
	VFrame *get_window_icon();

	EDLSession *vedlsession;
	VTracking *playback_cursor;

	VWindow *vwindow;
	VWindowGUI *gui;
	VPlayback *playback_engine;
};

#endif
