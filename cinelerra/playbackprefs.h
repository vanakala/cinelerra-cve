// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLAYBACKPREFS_H
#define PLAYBACKPREFS_H

class PlaybackLanczosLanczos;
class PlaybackBicubicBicubic;
class PlaybackBicubicBilinear;
class PlaybackBilinearBilinear;
class PlaybackBufferBytes;
class PlaybackBufferSize;
class PlaybackDisableNoEdits;
class PlaybackHead;
class PlaybackHeadCount;
class PlaybackHost;
class PlaybackNearest;
class PlaybackOutBits;
class PlaybackOutChannels;
class PlaybackOutPath;
class PlaybackPreload;
class PlaybackReadLength;
class TimecodeOffset;
class VideoAsynchronous;

#include "adeviceprefs.h"
#include "preferencesthread.h"
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

	ADevicePrefs *audio_device;
	VDevicePrefs *video_device;

	PlaybackConfig *playback_config;
	BC_Title *framerate_title;
	BC_Title *playedframes_title;
	BC_Title *lateframes_title;
	BC_Title *avgdelay_title;
	PlaybackNearest *nearest_neighbor;
	PlaybackLanczosLanczos *lanczos_lanczos;
	PlaybackBicubicBicubic *cubic_cubic;
	PlaybackBicubicBilinear *cubic_linear;
	PlaybackBilinearBilinear *linear_linear;
	BC_Title *vdevice_title;
};


class PlaybackAudioOffset : public BC_TumbleTextBox
{
public:
	PlaybackAudioOffset(PreferencesWindow *pwindow, 
		PlaybackPrefs *subwindow,
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};


class PlaybackNearest : public BC_Radial
{
public:
	PlaybackNearest(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
	PlaybackPrefs *prefs;
};


class PlaybackLanczosLanczos : public BC_Radial
{
public:
	PlaybackLanczosLanczos(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
	PlaybackPrefs *prefs;
};


class PlaybackBicubicBicubic : public BC_Radial
{
public:
	PlaybackBicubicBicubic(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
	PlaybackPrefs *prefs;
};


class PlaybackBicubicBilinear : public BC_Radial
{
public:
	PlaybackBicubicBilinear(PreferencesWindow *pwindow, 
		PlaybackPrefs *prefs, 
		int value, 
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
	PlaybackPrefs *prefs;
};


class PlaybackBilinearBilinear : public BC_Radial
{
public:
	PlaybackBilinearBilinear(PreferencesWindow *pwindow, 
		PlaybackPrefs *prefs, 
		int value, 
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
	PlaybackPrefs *prefs;
};


class TimecodeOffset : public BC_TextBox
{
public:
	TimecodeOffset(int x, int y, PreferencesWindow *pwindow,
		PlaybackPrefs *playback, char *text, int unit);

	int handle_event();

	int unit;
	PlaybackPrefs *playback;
	PreferencesWindow *pwindow;
};

#endif
