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
	int set_strategy(int strategy);
	int get_buffer_bytes();

	static char* strategy_to_string(int strategy);
	void delete_strategy();


	ArrayList<PlaybackConfig*>* current_config_list();
	PlaybackConfig* current_config();
	void update(int interpolation);
	int draw_framerate();

	ADevicePrefs *audio_device;
	VDevicePrefs *video_device;
	ArrayList<BC_ListBoxItem*> strategies;

	BC_Title *framerate_title;
	PlaybackNearest *nearest_neighbor;
	PlaybackBicubicBicubic *cubic_cubic;
	PlaybackBicubicBilinear *cubic_linear;
	PlaybackBilinearBilinear *linear_linear;
	PlaybackDeblock *mpeg4_deblock;

	long current_head;
	BC_Title *head_title;
	BC_Title *host_title;
	BC_Title *head_count_title;
	BC_Title *vdevice_title;
	PlaybackHead *head_text;
	PlaybackHeadCount *head_count_text;
	PlaybackHost *host_text;
};


class PlaybackStrategy : public BC_PopupTextBox
{
public:
	PlaybackStrategy(PlaybackPrefs *prefs, int x, int y);
	int handle_event();
	
	PlaybackPrefs *prefs;
};

class PlaybackHead : public BC_TumbleTextBox
{
public:
	PlaybackHead(PlaybackPrefs *prefs, int x, int y);
	int handle_event();
	PlaybackPrefs *prefs;
};

class PlaybackHeadCount : public BC_TumbleTextBox
{
public:
	PlaybackHeadCount(PlaybackPrefs *prefs, int x, int y);
	int handle_event();
	PlaybackPrefs *prefs;
};

class PlaybackHost : public BC_TextBox
{
public:
	PlaybackHost(PlaybackPrefs *prefs, int x, int y);
	int handle_event();
	PlaybackPrefs *prefs;
};

class PlaybackBufferSize : public BC_TextBox
{
public:
	PlaybackBufferSize(int x, int y, PreferencesWindow *pwindow, PlaybackPrefs *playback, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackReadLength : public BC_TextBox
{
public:
	PlaybackReadLength(int x, int y, PreferencesWindow *pwindow, PlaybackPrefs *playback, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackModuleFragment : public BC_TextBox
{
public:
	PlaybackModuleFragment(int x, int y, PreferencesWindow *pwindow, PlaybackPrefs *playback, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackBufferBytes : public BC_Title
{
public:
	PlaybackBufferBytes(int x, int y, PreferencesWindow *pwindow, PlaybackPrefs *playback, char *text);
	
	int update_bytes();
	
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
	char string[1024];
};


class PlaybackDisableNoEdits : public BC_CheckBox
{
public:
	PlaybackDisableNoEdits(PreferencesWindow *pwindow, int value, int y);
	int handle_event();
	PreferencesWindow *pwindow;
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
	VideoEveryFrame(PreferencesWindow *pwindow, int x, int y);
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


#endif
