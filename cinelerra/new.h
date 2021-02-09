// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef NEW_H
#define NEW_H

#include "edl.inc"
#include "bchash.inc"
#include "formatpresets.h"
#include "mwindow.inc"
#include "selection.inc"
#include "thread.h"

class NewThread;
class NewWindow;
class NewPresets;

class New : public BC_MenuItem
{
public:
	New();

	int handle_event();

	void create_new_project();

	NewThread *thread;
	EDLSession *new_edlsession;

private:
	void create_new_edl();
};


class NewThread : public Thread
{
public:
	NewThread(New *new_project);
	~NewThread();

	void run();

	int auto_sizes;
	NewWindow *nwindow;
	New *new_project;
	Mutex *window_lock;
};


class NewWindow : public BC_Window
{
public:
	NewWindow(NewThread *new_thread, int x, int y);
	~NewWindow();

	void update();

	MWindow *mwindow;
	NewThread *new_thread;
	EDLSession *new_edlsession;
	BC_TextBox *atracks;
	BC_TextBox *achannels;
	BC_TextBox *sample_rate;
	BC_TextBox *vtracks;
	BC_TextBox *vchannels;
	BC_TextBox *frame_rate;
	AspectRatioSelection *aspectratio_selection;
	FrameSizeSelection *framesize_selection;
	InterlaceModeSelection *interlace_selection;
	ColormodelSelection *cmodel_selection;
	NewPresets *format_presets;
};


class NewPresets : public FormatPresets
{
public:
	NewPresets(NewWindow *gui, int x, int y);

	int handle_event();
private:
	NewWindow *new_gui;
};


class NewATracks : public BC_TextBox
{
public:
	NewATracks(NewWindow *nwindow, const char *text, int x, int y);

	int handle_event();

	NewWindow *nwindow;
};

class NewATracksTumbler : public BC_Tumbler
{
public:
	NewATracksTumbler(NewWindow *nwindow, int x, int y);

	void handle_up_event();
	void handle_down_event();

	NewWindow *nwindow;
};

class NewAChannels : public BC_TextBox
{
public:
	NewAChannels(NewWindow *nwindow, const char *text, int x, int y);

	int handle_event();

	NewWindow *nwindow;
};


class NewAChannelsTumbler : public BC_Tumbler
{
public:
	NewAChannelsTumbler(NewWindow *nwindow, int x, int y);

	void handle_up_event();
	void handle_down_event();

	NewWindow *nwindow;
};


class NewVTracks : public BC_TextBox
{
public:
	NewVTracks(NewWindow *nwindow, const char *text, int x, int y);
	int handle_event();
	NewWindow *nwindow;
};


class NewVTracksTumbler : public BC_Tumbler
{
public:
	NewVTracksTumbler(NewWindow *nwindow, int x, int y);

	void handle_up_event();
	void handle_down_event();

	NewWindow *nwindow;
};

#endif
