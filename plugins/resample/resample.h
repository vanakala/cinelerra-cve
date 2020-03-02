
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

#ifndef RESAMPLEEFFECT_H
#define RESAMPLEEFFECT_H

#define PLUGIN_IS_AUDIO
#define PLUGIN_TITLE N_("Resample")
#define PLUGIN_CLASS ResampleEffect
#define PLUGIN_GUI_CLASS ResampleWindow

#include "pluginmacros.h"

#include "bctextbox.h"
#include "bchash.inc"
#include "language.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include "resample.inc"
#include "vframe.inc"

class ResampleFraction : public BC_TextBox
{
public:
	ResampleFraction(ResampleEffect *plugin, int x, int y);
	int handle_event();
	ResampleEffect *plugin;
};

class ResampleWindow : public PluginWindow
{
public:
	ResampleWindow(ResampleEffect *plugin, int x, int y);

	PLUGIN_GUI_CLASS_MEMBERS
};

class ResampleEffect : public PluginAClient
{
public:
	ResampleEffect(PluginServer *server);
	~ResampleEffect();

	PLUGIN_CLASS_MEMBERS;

	int process_loop(AFrame *buffer);
	void load_defaults();
	void save_defaults();

	Resample *resample;
	double scale;
	MainProgressBar *progress;
	AFrame *input_frame;
	samplenum total_written;
	samplenum predicted_total;
	ptstime current_pts;
	ptstime output_pts;
};

#endif
