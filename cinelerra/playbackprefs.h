#ifndef PLAYBACKPREFS_H
#define PLAYBACKPREFS_H

class PlaybackOutPath;
class PlaybackOutBits;
class PlaybackBufferSize;
class PlaybackBufferBytes;
class PlaybackOutChannels;
class PlaybackViewFollows;
class PlaybackRealTime;
class PlaybackSoftwareTimer;
class PlaybackModuleFragment;
class PlaybackReadLength;
class PlaybackDisableNoEdits;
class PlaybackPreload;
class PlaybackNearest;
class PlaybackBicubicBicubic;
class PlaybackBicubicBilinear;
class PlaybackBilinearBilinear;
class PlaybackDeblock;
class PlaybackHead;
class PlaybackHeadCount;
class PlaybackHost;
class TimecodeOffset;


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

class VideoEveryFrame : public BC_CheckBox
{
public:
	VideoEveryFrame(PreferencesWindow *pwindow, 
		int x, 
		int y);
	int handle_event();
	PreferencesWindow *pwindow;
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
