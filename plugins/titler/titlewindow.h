// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TITLEWINDOW_H
#define TITLEWINDOW_H

#include "colorpicker.h"
#include "filexml.h"
#include "mutex.h"
#include "title.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

class TitleFontTumble;
class TitleItalic;
class TitleBold;
class TitleSize;
class TitleColorButton;
class TitleColorStrokeButton;
class TitleStroke;
class TitleStrokeW;
class TitleDropShadow;
class TitleMotion;
class TitleLoop;
class TitleFade;
class TitleFont;
class TitleText;
class TitleX;
class TitleY;
class TitleLeft;
class TitleCenter;
class TitleRight;class TitleTop;
class TitleMid;
class TitleBottom;
class TitleColorThread;
class TitleColorStrokeThread;
class TitleSpeed;
class TitleTimecode;
class TitleTimecodeFormat;


class TitleWindow : public PluginWindow
{
public:
	TitleWindow(TitleMain *plugin, int x, int y);
	~TitleWindow();

	void update_color();
	void update_justification();
	void update();
	void previous_font();
	void next_font();

	BC_Title *font_title;
	TitleFont *font;
	TitleFontTumble *font_tumbler;
	BC_Title *x_title;
	TitleX *title_x;
	BC_Title *y_title;
	TitleY *title_y;
	BC_Title *dropshadow_title;
	TitleDropShadow *dropshadow;
	BC_Title *style_title;
	TitleItalic *italic;
	TitleBold *bold;
	TitleStroke *stroke;
	TitleColorStrokeButton *color_stroke_button;
	TitleColorStrokeThread *color_stroke_thread;
	BC_Title *strokewidth_title;
	TitleStrokeW *stroke_width;
	int color_stroke_x, color_stroke_y;
	int color_x, color_y;
	BC_Title *size_title;
	TitleSize *size;
	TitleColorButton *color_button;
	TitleColorThread *color_thread;
	BC_Title *motion_title;
	TitleMotion *motion;
	TitleLoop *loop;
	BC_Title *fadein_title;
	TitleFade *fade_in;
	BC_Title *fadeout_title;
	TitleFade *fade_out;
	BC_Title *text_title;
	TitleText *text;
	BC_Title *justify_title;
	TitleLeft *left;
	TitleCenter *center;
	TitleRight *right;
	TitleTop *top;
	TitleMid *mid;
	TitleBottom *bottom;
	BC_Title *speed_title;
	TitleSpeed *speed;
	TitleTimecode *timecode;
	TitleTimecodeFormat *timecodeformat;

// Color preview
	ArrayList<BC_ListBoxItem*> sizes;
	ArrayList<BC_ListBoxItem*> paths;
	ArrayList<BC_ListBoxItem*> fonts;
	ArrayList<BC_ListBoxItem*> timecodeformats;
	PLUGIN_GUI_CLASS_MEMBERS
};


class TitleFontTumble : public BC_Tumbler
{
public:
	TitleFontTumble(TitleMain *client, TitleWindow *window, int x, int y);

	void handle_up_event();
	void handle_down_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleItalic : public BC_CheckBox
{
public:
	TitleItalic(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleBold : public BC_CheckBox
{
public:
	TitleBold(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleStroke : public BC_CheckBox
{
public:
	TitleStroke(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};


class TitleSize : public BC_PopupTextBox
{
public:
	TitleSize(TitleMain *client, TitleWindow *window, int x, int y, char *text);

	int handle_event();
	void update(int size);

	TitleMain *client;
	TitleWindow *window;
};

class TitleColorButton : public BC_GenericButton
{
public:
	TitleColorButton(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};
class TitleColorStrokeButton : public BC_GenericButton
{
public:
	TitleColorStrokeButton(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};
class TitleMotion : public BC_PopupTextBox
{
public:
	TitleMotion(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleLoop : public BC_CheckBox
{
public:
	TitleLoop(TitleMain *client, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleTimecode : public BC_CheckBox
{
public:
	TitleTimecode(TitleMain *client, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleTimecodeFormat : public BC_PopupTextBox
{
public:
	TitleTimecodeFormat(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleFade : public BC_TextBox
{
public:
	TitleFade(TitleMain *client, TitleWindow *window, double *value, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
	double *value;
};

class TitleFont : public BC_PopupTextBox
{
public:
	TitleFont(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleText : public BC_ScrollTextBox
{
public:
	TitleText(TitleMain *client, 
		TitleWindow *window, 
		int x, 
		int y, 
		int w, 
		int h);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleX : public BC_TumbleTextBox
{
public:
	TitleX(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleY : public BC_TumbleTextBox
{
public:
	TitleY(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleStrokeW : public BC_TumbleTextBox
{
public:
	TitleStrokeW(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleDropShadow : public BC_TumbleTextBox
{
public:
	TitleDropShadow(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleSpeed : public BC_TumbleTextBox
{
public:
	TitleSpeed(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();
	TitleMain *client;
};

class TitleLeft : public BC_Radial
{
public:
	TitleLeft(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleCenter : public BC_Radial
{
public:
	TitleCenter(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleRight : public BC_Radial
{
public:
	TitleRight(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleTop : public BC_Radial
{
public:
	TitleTop(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleMid : public BC_Radial
{
public:
	TitleMid(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleBottom : public BC_Radial
{
public:
	TitleBottom(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_event();

	TitleMain *client;
	TitleWindow *window;
};

class TitleColorThread : public ColorThread
{
public:
	TitleColorThread(TitleMain *client, TitleWindow *window);

	int handle_new_color(int red, int green, int blue, int alpha);

	TitleMain *client;
	TitleWindow *window;
};

class TitleColorStrokeThread : public ColorThread
{
public:
	TitleColorStrokeThread(TitleMain *client, TitleWindow *window);

	int handle_new_color(int red, int green, int blue, int alpha);

	TitleMain *client;
	TitleWindow *window;
};

#endif
