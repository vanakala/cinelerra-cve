#ifndef AUDIOINPREFS_H
#define AUDIOINPREFS_H

class DuplexEnable;
class RecordMinDB;
class RecordVUDB;
class RecordVUInt;
class RecordWriteLength;
class RecordRealTime;

#include "mwindow.inc"
#include "preferencesthread.h"
#include "adeviceprefs.inc"


class AudioInPrefs : public PreferencesDialog
{
public:
	AudioInPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~AudioInPrefs();

	int create_objects();

	ADevicePrefs *in_device, *duplex_device;
	MWindow *mwindow;
};


class RecordWriteLength : public BC_TextBox
{
public:
	RecordWriteLength(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
};

class DuplexEnable : public BC_CheckBox
{
public:
	DuplexEnable(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value);
	int handle_event();
	PreferencesWindow *pwindow;
};

class RecordRealTime : public BC_CheckBox
{
public:
	RecordRealTime(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value);
	int handle_event();
	PreferencesWindow *pwindow;
};

#endif
