#ifndef RECORDPREFS_H
#define RECORDPREFS_H

//class DuplexEnable;
class RecordMinDB;
class RecordVUDB;
class RecordVUInt;
class RecordWriteLength;
class RecordRealTime;

#include "adeviceprefs.inc"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "recordprefs.inc"
#include "vdeviceprefs.inc"

class RecordPrefs : public PreferencesDialog
{
public:
	RecordPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~RecordPrefs();

	int create_objects();

	ADevicePrefs *in_device /*, *duplex_device */;
	VDevicePrefs *video_in_device;
	MWindow *mwindow;
};


class RecordWriteLength : public BC_TextBox
{
public:
	RecordWriteLength(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
};

/*
 * class DuplexEnable : public BC_CheckBox
 * {
 * public:
 * 	DuplexEnable(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value);
 * 	int handle_event();
 * 	PreferencesWindow *pwindow;
 * };
 * 
 */
class RecordRealTime : public BC_CheckBox
{
public:
	RecordRealTime(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value);
	int handle_event();
	PreferencesWindow *pwindow;
};


class RecordSampleRate : public BC_TextBox
{
public:
	RecordSampleRate(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
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

class RecordFrameRate : public BC_TextBox
{
public:
	RecordFrameRate(PreferencesWindow *pwindow, int x, int y);
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
