
/*
 * CINELERRA
 * Copyright (C) 2017 Einar Rünkaru <einarrunkaru@gmail dot com>
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
