
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
class PlaybackSoftwareTimer;
class PlaybackViewFollows;
class TimecodeOffset;
class VideoAsynchronous;

#include "adeviceprefs.h"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "vdeviceprefs.h"

class PlaybackPrefs : public PreferencesDialog
{
public:
	PlaybackPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
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


class PlaybackViewFollows : public BC_CheckBox
{
public:
	PlaybackViewFollows(PreferencesWindow *pwindow, int value, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PlaybackSoftwareTimer : public BC_CheckBox
{
public:
	PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class VideoEveryFrame : public BC_CheckBox
{
public:
	VideoEveryFrame(PreferencesWindow *pwindow, 
		PlaybackPrefs *playback_prefs,
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
	PlaybackPrefs *playback_prefs;
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
