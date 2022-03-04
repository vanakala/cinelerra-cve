// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SETFORMAT_H
#define SETFORMAT_H

#include "edlsession.inc"
#include "formatpresets.h"
#include "mutex.inc"
#include "rotateframe.inc"
#include "setformat.inc"
#include "selection.inc"
#include "thread.h"


class SetFormatPresets;


class SetFormat : public BC_MenuItem
{
public:
	SetFormat();
	~SetFormat();

	int handle_event();

	SetFormatThread *thread;
};


class SetFormatThread : public Thread
{
public:
	SetFormatThread();
	~SetFormatThread();

	void run();

	void apply_changes();
// Update image size based on ratio of dimensions to original.
	void update_window();

// Update all parameters from preset menu
	void update();

	Mutex *window_lock;
	SetFormatWindow *window;
	EDLSession *new_edlsession;
	double ratio[2];
	int dimension[2];
	int orig_dimension[2];
};

class SetChannelsTextBox : public BC_TextBox
{
public:
	SetChannelsTextBox(SetFormatThread *thread, int x, int y);

	int handle_event();

	SetFormatThread *thread;
};


class SetChannelsCanvas : public BC_SubWindow
{
public:
	SetChannelsCanvas(SetFormatThread *thread,
		int x, 
		int y,
		int w,
		int h);
	~SetChannelsCanvas();

	void draw(int angle = -1);
	void get_dimensions(int channel_position, int &x, int &y, int &w, int &h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();

private:
	int active_channel;   // for selection
	int degree_offset;
	int box_r;

	int poltoxy(int &x, int &y, int r, int d);
	int xytopol(int &d, int x, int y);
	SetFormatThread *thread;
	VFrame *temp_picon;
	RotateFrame *rotater;
};


class ScaleRatioText : public BC_TextBox
{
public:
	ScaleRatioText(int x, int y, SetFormatThread *thread, double *output);

	int handle_event();

	SetFormatThread *thread;
	double *output;
};


class SetFormatApply : public BC_GenericButton
{
public:
	SetFormatApply(int x, int y, SetFormatThread *thread);

	int handle_event();
	SetFormatThread *thread;
};


class SetFormatPresets : public FormatPresets
{
public:
	SetFormatPresets(SetFormatWindow *gui, int x, int y);

	int handle_event();
private:
	SetFormatWindow *format_gui;
};


class SetFrameSize : public FrameSizeSelection
{
public:
	SetFrameSize(int x1, int y1, int x2, int y2,
		BC_WindowBase *base, int *value1, int *value2, SetFormatThread *thread);

	int handle_event();

	int oldvalue1;
	int oldvalue2;
	SetFormatThread *thread;
};


class SetFormatWindow : public BC_Window
{
public:
	SetFormatWindow(SetFormatThread *thread,
		int x,
		int y);
	~SetFormatWindow();

	SetFormatThread *thread;
	SetChannelsCanvas *canvas;

	SetFrameSize *framesize_selection;
	SetFormatPresets *presets;

// Size ratio width, height
	ScaleRatioText* ratio[2];
	AspectRatioSelection *aspectratio_selection;
	Selection *sample_rate;
	SetChannelsTextBox *channels;
	FrameRateSelection *frame_rate;
	ColormodelSelection *cmodel_selection;
	InterlaceModeSelection *interlace_selection;
};

#endif
