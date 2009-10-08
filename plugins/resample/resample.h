
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


#include "bchash.inc"
#include "guicast.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "resample.inc"
#include "vframe.inc"

class ResampleEffect;


class ResampleFraction : public BC_TextBox
{
public:
	ResampleFraction(ResampleEffect *plugin, int x, int y);
	int handle_event();
	ResampleEffect *plugin;
};



class ResampleWindow : public BC_Window
{
public:
	ResampleWindow(ResampleEffect *plugin, int x, int y);
	void create_objects();
	ResampleEffect *plugin;
};



class ResampleEffect : public PluginAClient
{
public:
	ResampleEffect(PluginServer *server);
	~ResampleEffect();

	char* plugin_title();
	int get_parameters();
	VFrame* new_picon();
	int start_loop();
	int process_loop(double *buffer, int64_t &write_length);
	int stop_loop();
	int load_defaults();
	int save_defaults();
	void reset();


	Resample *resample;
	double scale;
	BC_Hash *defaults;
	MainProgressBar *progress;
	int64_t total_written;
	int64_t current_position;
};





#endif
