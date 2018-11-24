
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

#ifndef NEW_H
#define NEW_H

#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
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
	New(MWindow *mwindow);

	int handle_event();
	int run_script(FileXML *script);
	void create_new_project();

	MWindow *mwindow;
	NewThread *thread;
	EDL *new_edl;

private:
	void create_new_edl();
	FileXML *script;
};


class NewThread : public Thread
{
public:
	NewThread(MWindow *mwindow, New *new_project);
	~NewThread();

	void run();

	int auto_sizes;
	NewWindow *nwindow;
	MWindow *mwindow;
	New *new_project;
	Mutex *window_lock;
};


class NewWindow : public BC_Window
{
public:
	NewWindow(MWindow *mwindow, NewThread *new_thread, int x, int y);
	~NewWindow();

	void update();

	MWindow *mwindow;
	NewThread *new_thread;
	EDL *new_edl;
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
