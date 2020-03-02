
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

#ifndef NORMALIZE_H
#define NORMALIZE_H

#define PLUGIN_TITLE N_("Normalize")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_CLASS NormalizeMain
#define PLUGIN_GUI_CLASS NormalizeWindow

#include "pluginmacros.h"
#include "bchash.inc"
#include "cinelerra.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "vframe.inc"


class NormalizeMain : public PluginAClient
{
public:
	NormalizeMain(PluginServer *server);
	~NormalizeMain();

	PLUGIN_CLASS_MEMBERS

// normalizing engine

// parameters needed
	float db_over;
	int separate_tracks;

	int process_loop(AFrame **buffer);

	void load_defaults();
	void save_defaults();

	MainProgressBar *progress;

// Current state of process_loop
	int writing;
	ptstime current_pts;
	double peak[MAXCHANNELS], scale[MAXCHANNELS];
};

#endif
