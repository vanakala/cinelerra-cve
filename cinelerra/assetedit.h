// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ASSETEDIT_H
#define ASSETEDIT_H

#include "asset.inc"
#include "awindow.inc"
#include "bcbutton.h"
#include "bcpopupmenu.h"
#include "browsebutton.h"
#include "cinelerra.h"
#include "datatype.h"
#include "formatpresets.h"
#include "language.h"
#include "paramlist.h"
#include "paramlistwindow.inc"
#include "thread.h"

class AssetEditPath;
class AssetEditPathText;
class AssetEditWindow;
class AssetInterlaceMode;
class AssetInfoWindow;
class AssetEditConfigButton;


class AssetEdit : public Thread
{
public:
	AssetEdit();

	void edit_asset(Asset *asset);
	void run();

	Asset *asset, *new_asset;
	AssetEditWindow *window;
};


// Pcm is the only format users should be able to fix.
// All other formats display information about the file in read-only.

class AssetEditWindow : public BC_Window
{
public:
	AssetEditWindow(AssetEdit *asset_edit, int absx, int absy);
	~AssetEditWindow();

	Asset *asset;
	AssetEditPath *path_button;
	AssetEdit *asset_edit;
	AssetInterlaceMode *ilacemode_selection;
	InterlaceFixSelection *ilacefix_selection;
private:
	AssetInfoWindow *aiwin[MAXCHANNELS];
	AssetInfoWindow *viwin[MAXCHANNELS];
	int numaudio;
	int numvideo;
	AssetEditConfigButton *fmt_button;
};

class AssetInfoWindow : public BC_SubWindow
{
public:
	AssetInfoWindow(struct streamdesc *dsc,
		int edit_font_color);

	virtual void draw_window() {};
	void cancel_window();

	int width;
	int height;

protected:
	void show_line(const char *prompt, const char *value);
	void show_line(const char *prompt, int value);
	void show_line(const char *prompt, double value);
	void show_line(const char *prompt, ptstime start, ptstime end);
	void show_streamnum();
	void show_button(Paramlist *params);

	int stnum;
	int font_color;
	int p_prompt;
	int p_value;
	int last_line_pos;
	struct streamdesc *desc;
	AssetEditConfigButton *button;
};

class AudioInfoWindow : public AssetInfoWindow
{
public:
	AudioInfoWindow(struct streamdesc *dsc,
		int edit_font_color);

	void draw_window();
};

class VideoInfoWindow : public AssetInfoWindow
{
public:
	VideoInfoWindow(struct streamdesc *dsc,
		int edit_font_color);

	void draw_window();
};

class AssetEditPath : public BrowseButton
{
public:
	AssetEditPath(AssetEditWindow *fwindow,
		BC_TextBox *textbox,
		int x,
		int y,
		const char *text,
		const char *window_title,
		const char *window_caption);

	AssetEditWindow *fwindow;
};


class AssetEditPathText : public BC_TextBox
{
public:
	AssetEditPathText(AssetEditWindow *fwindow, int x, int y, int w);

	int handle_event();

	AssetEditWindow *fwindow;
};


class Interlaceautofix : public BC_CheckBox
{
public:
	Interlaceautofix(AssetEditWindow *fwindow, int x, int y);

	int handle_event();

	void showhideotherwidgets();

	AssetEditWindow* fwindow;
};


class AssetInterlaceMode : public AInterlaceModeSelection
{
public:
	AssetInterlaceMode(int x, int y, BC_WindowBase *base_gui, int *value);

	int handle_event();

	Interlaceautofix *autofix;
};

class AssetEditConfigButton : public BC_Button
{
public:
	AssetEditConfigButton(int x, int y, Paramlist *params);
	~AssetEditConfigButton();

	int handle_event();
	void cancel_window();

private:
	Paramlist *list;
	ParamlistThread *thread;
};

#endif
