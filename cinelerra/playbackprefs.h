
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

class PlaybackBicubicBicubic;
class PlaybackBicubicBilinear;
class PlaybackBilinearBilinear;
class PlaybackBufferBytes;
class PlaybackBufferSize;
class PlaybackDeblock;
class PlaybackDisableNoEdits;
class PlaybackHead;
class PlaybackHeadCount;
class PlaybackHost;
class PlaybackInterpolateRaw;
class PlaybackModuleFragment;
class PlaybackNearest;
class PlaybackOutBits;
class PlaybackOutChannels;
class PlaybackOutPath;
class PlaybackPreload;
class PlaybackReadLength;
class PlaybackRealTime;
class PlaybackSoftwareTimer;
class PlaybackViewFollows;
class PlaybackWhiteBalanceRaw;
class TimecodeOffset;
class VideoAsynchronous;

#include "adeviceprefs.h"
#include "guicast.h"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "vdeviceprefs.h"

class PlaybackPrefs : public PreferencesDialog
{
public:
	PlaybackPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~PlaybackPrefs();

	int create_objects();
//	int set_strategy(int strategy);
	int get_buffer_bytes();

	static char* strategy_to_string(int strategy);
	void delete_strategy();

	void update(int interpolation);
	int draw_framerate();

	ADevicePrefs *audio_device;
	VDevicePrefs *video_device;
	ArrayList<BC_ListBoxItem*> strategies;

	PlaybackConfig *playback_config;
	BC_Title *framerate_title;
	PlaybackNearest *nearest_neighbor;
	PlaybackBicubicBicubic *cubic_cubic;
	PlaybackBicubicBilinear *cubic_linear;
	PlaybackBilinearBilinear *linear_linear;
	PlaybackDeblock *mpeg4_deblock;
	PlaybackInterpolateRaw *interpolate_raw;
	PlaybackWhiteBalanceRaw *white_balance_raw;
	VideoAsynchronous *asynchronous;

	BC_Title *vdevice_title;
};

class PlaybackModuleFragment : public BC_PopupMenu
{
public:
	PlaybackModuleFragment(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PlaybackPrefs *playback, 
		char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
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

class PlaybackRealTime : public BC_CheckBox
{
public:
	PlaybackRealTime(PreferencesWindow *pwindow, int value, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class VideoAsynchronous : public BC_CheckBox
{
public:
	VideoAsynchronous(PreferencesWindow *pwindow, int x, int y);
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

class PlaybackDeblock : public BC_CheckBox
{
public:
	PlaybackDeblock(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PlaybackNearest : public BC_Radial
{
public:
	PlaybackNearest(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y);

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

class PlaybackPreload : public BC_TextBox
{
public:
	PlaybackPreload(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PlaybackPrefs *playback, 
		char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackInterpolateRaw : public BC_CheckBox
{
public:
	PlaybackInterpolateRaw(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackWhiteBalanceRaw : public BC_CheckBox
{
public:
	PlaybackWhiteBalanceRaw(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
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


class PlaybackSubtitle : public BC_CheckBox
{
public:
	PlaybackSubtitle(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackSubtitleNumber : public BC_TumbleTextBox
{
public:
	PlaybackSubtitleNumber(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};


#endif
