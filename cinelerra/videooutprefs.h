#ifndef VIDEOOUTPREFS_H
#define VIDEOOUTPREFS_H

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
class PlaybackBicubic;

#include "adeviceprefs.h"
#include "guicast.h"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "vdeviceprefs.h"

class PlaybackHead;
class PlaybackHeadCount;
class PlaybackHost;

class VideoOutPrefs : public PreferencesDialog
{
public:
	VideoOutPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~VideoOutPrefs();

	int create_objects();
	int set_strategy(int strategy, int y);
	int get_buffer_bytes();

	static char* strategy_to_string(int strategy);
	void delete_strategy();


	ArrayList<PlaybackConfig*>* current_config_list();
	PlaybackConfig* current_config();
	void update(int interpolation);

	VDevicePrefs *video_device;
	ArrayList<BC_ListBoxItem*> strategies;

	long current_head;
	BC_Title *head_title;
	BC_Title *host_title;
	BC_Title *head_count_title;
	BC_Title *vdevice_title;
	BC_Title *framerate_title;
	PlaybackHead *head_text;
	PlaybackHeadCount *head_count_text;
	PlaybackHost *host_text;
	PlaybackNearest *nearest_neighbor;
	PlaybackBicubic *bicubic;
};

class VideoEveryFrame : public BC_CheckBox
{
public:
	VideoEveryFrame(PreferencesWindow *pwindow, int x, int y);
	~VideoEveryFrame();
	int handle_event();
	PreferencesWindow *pwindow;
};

class PlaybackNearest : public BC_Radial
{
public:
	PlaybackNearest(PreferencesWindow *pwindow, VideoOutPrefs *prefs, int value, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
	VideoOutPrefs *prefs;
};

class PlaybackBicubic : public BC_Radial
{
public:
	PlaybackBicubic(PreferencesWindow *pwindow, VideoOutPrefs *prefs, int value, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
	VideoOutPrefs *prefs;
};

class PlaybackPreload : public BC_TextBox
{
public:
	PlaybackPreload(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		VideoOutPrefs *playback, 
		char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	VideoOutPrefs *playback;
};


#endif
