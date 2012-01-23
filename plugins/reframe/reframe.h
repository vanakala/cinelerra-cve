
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

#ifndef REFRAME_H
#define REFRAME_H

#define PLUGIN_IS_VIDEO

#define PLUGIN_TITLE N_("Reframe")
#define PLUGIN_CLASS ReFrame
#define PLUGIN_GUI_CLASS ReFrameWindow

#include "pluginmacros.h"

#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"


class ReFrameOutput : public BC_TextBox
{
public:
	ReFrameOutput(ReFrame *plugin, int x, int y);
	int handle_event();
	ReFrame *plugin;
};


class ReFrameWindow : public BC_Window
{
public:
	ReFrameWindow(ReFrame *plugin, int x, int y);
	~ReFrameWindow();

	PLUGIN_GUI_CLASS_MEMBERS
};


class ReFrame : public PluginVClient
{
public:
	ReFrame(PluginServer *server);
	~ReFrame();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void start_loop();
	void stop_loop();
	int process_loop(VFrame *buffer);

	double scale;

	MainProgressBar *progress;
	ptstime current_pts;
};

#endif
