
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

#include "asset.h"
#include "language.h"
#include "mwindow.h"
#include "bctitle.h"
#include "bcrecentlist.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctextbox.h"
#include "formattools.h"
#include "pipeconfig.h"

#include <string.h>

// notes for presets:
//  the '%' character will be replaced by the current path
//  to insert a real '%' double it up: '%%' -> '%'
//  preset items must have a '|' before the actual command

const char *YUVPreset::presets[] =
{
	"(DVD) | ffmpeg -f yuv4mpegpipe -i - -y -target dvd -ilme -ildct -hq -f mpeg2video %",
	"(VCD) | ffmpeg -f yuv4mpegpipe -i - -y -target vcd -hq -f mpeg2video %",
	"(DVD) | mpeg2enc -f 8 -o %",
	"(VCD) | mpeg2enc -f 2 -o %",
	0
};

PipeConfigWindow::PipeConfigWindow(int x, int y, Asset *asset)
 : BC_SubWindow(x, y, 200, 200)
{
	win_x = x;
	win_y = y;
	this->asset = asset;
}

void PipeConfigWindow::draw_window()
{
	BC_Title *bt;
	YUVPreset *pr;
	PipeButton *but;
	int x, y, w, x0;

	x = PIPECONFIG_MARGIN;
	y = w = 0;

	add_subwindow(bt = new BC_Title(x, y, _("Use Pipe:")));
	w = x + bt->get_w() + 5;
	add_subwindow(pipe_checkbox = new PipeCheckBox(w, y,
		asset->use_pipe));
	x0 = w += pipe_checkbox->get_w() + 5;
	add_subwindow(pipe_textbox = new BC_TextBox(w, y, 350, 1, asset->pipe));
	w += pipe_textbox->get_w();
	add_subwindow(pipe_recent = new BC_RecentList("PIPE", mwindow_global->defaults,
		pipe_textbox, 10, w, y, pipe_textbox->get_w(), 100));
	pipe_recent->load_items("YUV");
	w += pipe_recent->get_w();
	if(!asset->use_pipe)
		pipe_textbox->disable();
	add_subwindow(pr = new YUVPreset(x0, y + pipe_recent->get_h(),
		pipe_textbox, pipe_checkbox));
	add_subwindow(but = new PipeButton(w, y, pr));
	y += pipe_recent->get_h(); // see
	w += pr->get_w();
	pipe_checkbox->textbox = pipe_textbox;
	w += but->get_w() + PIPECONFIG_MARGIN;
	reposition_window(win_x, win_y, w, y);
}

void PipeConfigWindow::save_defaults()
{
	pipe_recent->add_item("YUV", asset->pipe);
}

YUVPreset::YUVPreset(int x, int y, BC_TextBox *textbox, PipeCheckBox *checkbox)
 : BC_PopupMenu(x, y, 0, "", POPUPMENU_USE_COORDS)
{
	this->pipe_textbox = textbox;
	this->pipe_checkbox = checkbox;
	for(int i = 0; presets[i]; i++)
		add_item(new PresetItem(presets[i], textbox));
}

int YUVPreset::handle_event()
{
	pipe_textbox->enable();
	pipe_checkbox->set_value(1, 1);
	return 1;
}

PresetItem::PresetItem(const char *text, BC_TextBox *textbox)
 : BC_MenuItem(text)
{
	this->textbox = textbox;
}

int PresetItem::handle_event()
{
	char *text = get_text();
	char *pipe = strchr(text, '|');

	if(pipe++)
	{
		while(*pipe && *pipe == ' ')
			pipe++;
		textbox->update(pipe);
	}
	return 1;
}

PipeCheckBox::PipeCheckBox(int x, int y, int value)
 : BC_CheckBox(x, y, value)
{
	this->textbox = 0;
}

int PipeCheckBox::handle_event()
{
	if(textbox)
	{
		if(get_value())
			textbox->enable();
		else
			textbox->disable();
	}
	return 1;
}

PipeButton::PipeButton(int x, int y, YUVPreset *preset)
 : BC_Button(x, y, get_resources()->listbox_button)
{
	this->preset = preset;
	set_tooltip(_("Presets"));
}

int PipeButton::handle_event()
{
	preset->activate_menu(1);
	return 1;
}
