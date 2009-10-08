
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

#ifndef SETFORMAT_H
#define SETFORMAT_H


#include "edl.inc"
#include "formatpresets.h"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "setformat.inc"
#include "thread.h"


class SetFormatPresets;



class SetFormat : public BC_MenuItem
{
public:
	SetFormat(MWindow *mwindow);
	int handle_event();
	SetFormatThread *thread;
	MWindow *mwindow;
};


class SetFormatThread : public Thread
{
public:
	SetFormatThread(MWindow *mwindow);

	void run();

	void apply_changes();
// Update image size based on ratio of dimensions to original.
	void update_window();
// Update automatic aspect ratio based in image size
	void update_aspect();
// Update all parameters from preset menu
	void update();


	Mutex *window_lock;
	SetFormatWindow *window;
	MWindow *mwindow;
	EDL *new_settings;
	float ratio[2];
	int dimension[2];
	int orig_dimension[2];
	int auto_aspect;
	int constrain_ratio;
};


class SetSampleRateTextBox : public BC_TextBox
{
public:
	SetSampleRateTextBox(SetFormatThread *thread, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};

class SetChannelsTextBox : public BC_TextBox
{
public:
	SetChannelsTextBox(SetFormatThread *thread, int x, int y);
	
	int handle_event();
	
	SetFormatThread *thread;
	MWindow *mwindow;
};


class SetChannelsCanvas : public BC_SubWindow
{
public:
	SetChannelsCanvas(MWindow *mwindow, 
		SetFormatThread *thread, 
		int x, 
		int y,
		int w,
		int h);
	~SetChannelsCanvas();
	
	int draw(int angle = -1);
	int get_dimensions(int channel_position, int &x, int &y, int &w, int &h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();

private:	
	int active_channel;   // for selection
	int degree_offset;
	int box_r;
	
	int poltoxy(int &x, int &y, int r, int d);
	int xytopol(int &d, int x, int y);
	MWindow *mwindow;
	SetFormatThread *thread;
	VFrame *temp_picon;
	RotateFrame *rotater;
};


class SetFrameRateTextBox : public BC_TextBox
{
public:
	SetFrameRateTextBox(SetFormatThread *thread, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};

class ScaleSizeText : public BC_TextBox
{
public:
	ScaleSizeText(int x, int y, SetFormatThread *thread, int *output);
	~ScaleSizeText();
	int handle_event();
	SetFormatThread *thread;
	int *output;
};


class ScaleRatioText : public BC_TextBox
{
public:
	ScaleRatioText(int x, int y, SetFormatThread *thread, float *output);
	~ScaleRatioText();
	int handle_event();
	SetFormatThread *thread;
	float *output;
};

class ScaleAspectAuto : public BC_CheckBox
{
public:
	ScaleAspectAuto(int x, int y, SetFormatThread *thread);
	~ScaleAspectAuto();
	int handle_event();
	SetFormatThread *thread;
};

class ScaleAspectText : public BC_TextBox
{
public:
	ScaleAspectText(int x, int y, SetFormatThread *thread, float *output);
	~ScaleAspectText();
	int handle_event();
	SetFormatThread *thread;
	float *output;
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
	SetFormatPresets(MWindow *mwindow, SetFormatWindow *gui, int x, int y);
	~SetFormatPresets();
	int handle_event();
	EDL* get_edl();
};

class FormatSwapExtents : public BC_Button
{
public:
	FormatSwapExtents(MWindow *mwindow, 
		SetFormatThread *thread,
		SetFormatWindow *gui, 
		int x, 
		int y);
	int handle_event();
	MWindow *mwindow;
	SetFormatThread *thread;
	SetFormatWindow *gui;
};

class SetFormatWindow : public BC_Window
{
public:
	SetFormatWindow(MWindow *mwindow, 
		SetFormatThread *thread,
		int x,
		int y);

	void create_objects();
	char* get_preset_text();

	MWindow *mwindow;
	SetFormatThread *thread;
	SetChannelsCanvas *canvas;
// Screen size width, height
	ScaleSizeText* dimension[2];
	SetFormatPresets *presets;
// Size ratio width, height
	ScaleRatioText* ratio[2];
// Aspect ratio
	ScaleAspectText *aspect_w;
	ScaleAspectText *aspect_h;
	SetSampleRateTextBox *sample_rate;
	SetChannelsTextBox *channels;
	SetFrameRateTextBox *frame_rate;
	BC_TextBox *color_model;
	ScaleAspectAuto *auto_aspect;
	InterlacemodePulldown *interlace_pulldown;
};

	
	
	




#endif
