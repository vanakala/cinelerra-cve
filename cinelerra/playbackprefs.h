// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLAYBACKPREFS_H
#define PLAYBACKPREFS_H

#include "adeviceprefs.h"
#include "preferencesthread.h"
#include "selection.h"
#include "vdeviceprefs.h"

class PlaybackPrefs : public PreferencesDialog
{
public:
	PlaybackPrefs(PreferencesWindow *pwindow);
	~PlaybackPrefs();

	void show();

	void update(int interpolation);
	void draw_framerate();
	void draw_playstatistics();
private:
	ADevicePrefs *audio_device;
	VDevicePrefs *video_device;

	PlaybackConfig *playback_config;
	BC_Title *framerate_title;
	BC_Title *playedframes_title;
	BC_Title *lateframes_title;
	BC_Title *avgdelay_title;
	BC_Title *vdevice_title;
	static const struct selection_int scalings[];
};

#endif
