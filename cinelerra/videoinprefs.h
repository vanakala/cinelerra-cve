#ifndef VIDEOINPREFS_H
#define VIDEOINPREFS_H

#include "mwindow.inc"
#include "preferencesthread.h"
#include "vdeviceprefs.inc"


class VideoInPrefs : public PreferencesDialog
{
public:
	VideoInPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~VideoInPrefs();

	int create_objects();

	VDevicePrefs *video_in_device;
	MWindow *mwindow;
};


class RecordSoftwareTimer : public BC_CheckBox
{
public:
	RecordSoftwareTimer(PreferencesWindow *pwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};


class RecordSyncDrives : public BC_CheckBox
{
public:
	RecordSyncDrives(PreferencesWindow *pwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class VideoWriteLength : public BC_TextBox
{
public:
	VideoWriteLength(PreferencesWindow *pwindow, char *text, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class VideoCaptureLength : public BC_TextBox
{
public:
	VideoCaptureLength(PreferencesWindow *pwindow, char *text, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class CaptureLengthTumbler : public BC_Tumbler
{
public:
	CaptureLengthTumbler(PreferencesWindow *pwindow, BC_TextBox *text, int x, int y);
	int handle_up_event();
	int handle_down_event();
	PreferencesWindow *pwindow;
	BC_TextBox *text;
};

class RecordW : public BC_TextBox
{
public:
	RecordW(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class RecordH : public BC_TextBox
{
public:
	RecordH(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class RecordFrameRateText : public BC_TextBox
{
	RecordFrameRateText(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

#endif
