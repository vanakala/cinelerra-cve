
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

#include "bchash.inc"
#include "guicast.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "vframe.inc"


class NormalizeMain : public PluginAClient
{
public:
	NormalizeMain(PluginServer *server);
	~NormalizeMain();

// normalizing engine

// parameters needed

	float db_over;
	int separate_tracks;

// required for all non realtime/multichannel plugins

	VFrame* new_picon();
	char* plugin_title();
	int is_realtime();
	int is_multichannel();
	int get_parameters();
	int start_loop();
	int process_loop(double **buffer, int64_t &write_length);
	int stop_loop();

	int load_defaults();  
	int save_defaults();  

	BC_Hash *defaults;
	MainProgressBar *progress;

// Current state of process_loop
	int writing;
	int64_t current_position;
	double *peak, *scale;
};


#endif
