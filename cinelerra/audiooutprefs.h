#ifndef AUDIOOUTPREFS_H
#define AUDIOOUTPREFS_H

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

#include "adeviceprefs.h"
#include "guicast.h"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "vdeviceprefs.h"

class PlaybackHead;
class PlaybackHeadCount;
class PlaybackHost;

class AudioOutPrefs : public PreferencesDialog
{
public:
	AudioOutPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~AudioOutPrefs();

	int create_objects();
	int set_strategy(int strategy);
	int get_buffer_bytes();

	static char* strategy_to_string(int strategy);
	void delete_strategy();


	ArrayList<PlaybackConfig*>* current_config_list();
	PlaybackConfig* current_config();

	ADevicePrefs *audio_device;
	VDevicePrefs *video_device;
	ArrayList<BC_ListBoxItem*> strategies;

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
	PlaybackStrategy(AudioOutPrefs *prefs, int x, int y);
	int handle_event();
	
	AudioOutPrefs *prefs;
};

class PlaybackHead : public BC_TumbleTextBox
{
public:
	PlaybackHead(AudioOutPrefs *prefs, int x, int y);
	int handle_event();
	AudioOutPrefs *prefs;
};

class PlaybackHeadCount : public BC_TumbleTextBox
{
public:
	PlaybackHeadCount(AudioOutPrefs *prefs, int x, int y);
	int handle_event();
	AudioOutPrefs *prefs;
};

class PlaybackHost : public BC_TextBox
{
public:
	PlaybackHost(AudioOutPrefs *prefs, int x, int y);
	int handle_event();
	AudioOutPrefs *prefs;
};

class PlaybackBufferSize : public BC_TextBox
{
public:
	PlaybackBufferSize(int x, int y, PreferencesWindow *pwindow, AudioOutPrefs *playback, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	AudioOutPrefs *playback;
};

class PlaybackReadLength : public BC_TextBox
{
public:
	PlaybackReadLength(int x, int y, PreferencesWindow *pwindow, AudioOutPrefs *playback, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	AudioOutPrefs *playback;
};

class PlaybackModuleFragment : public BC_TextBox
{
public:
	PlaybackModuleFragment(int x, int y, PreferencesWindow *pwindow, AudioOutPrefs *playback, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	AudioOutPrefs *playback;
};

class PlaybackBufferBytes : public BC_Title
{
public:
	PlaybackBufferBytes(int x, int y, PreferencesWindow *pwindow, AudioOutPrefs *playback, char *text);
	
	int update_bytes();
	
	PreferencesWindow *pwindow;
	AudioOutPrefs *playback;
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


#endif
