// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2017 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef PIPECONFIG_H
#define PIPECONFIG_H

#include "asset.inc"
#include "bcsubwindow.h"
#include "bctextbox.inc"
#include "bcpopupmenu.h"
#include "bcrecentlist.h"
#include "pipeconfig.inc"

class PipeCheckBox;

class PipeConfigWindow : public BC_SubWindow
{
public:
	PipeConfigWindow(int x, int y, Asset *asset);

	void save_defaults();

	void draw_window();
	PipeCheckBox *pipe_checkbox;
	BC_TextBox *pipe_textbox;
	BC_RecentList *pipe_recent;
private:
	int win_x;
	int win_y;
	Asset *asset;
};

class YUVPreset : public BC_PopupMenu
{
public:
	YUVPreset(int x, int y, BC_TextBox *textbox, PipeCheckBox *checkbox);

	int handle_event();
private:
	BC_TextBox *pipe_textbox;
	PipeCheckBox *pipe_checkbox;
	static const char *presets[];
};

class PresetItem : public BC_MenuItem
{
public:
	PresetItem(const char *text, BC_TextBox *textbox);

	int handle_event();
private:
	BC_TextBox *textbox;
};

class PipeCheckBox : public BC_CheckBox
{
public:
	PipeCheckBox(int x, int y, int value);

	int handle_event();

	BC_TextBox *textbox;
};

class PipeButton : public BC_Button
{
public:
	PipeButton(int x, int y, YUVPreset *preset);

	int handle_event();
private:
	YUVPreset *preset;
};

#endif
