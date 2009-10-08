
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

#ifndef OIL_H
#define OIL_H

class OilMain;

#include "bcbase.h"
#include "oilwindow.h"
#include "pluginvclient.h"

typedef struct
{
	float red;
    float green;
    float blue;
    float alpha;
} pixel_f;

class OilMain : public PluginVClient
{
public:
	OilMain(int argc, char *argv[]);
	~OilMain();

// required for all realtime plugins
	int process_realtime(long size, VFrame **input_ptr, VFrame **output_ptr);
	int plugin_is_realtime();
	int plugin_is_multi_channel();
	char* plugin_title();
	int start_gui();
	int stop_gui();
	int show_gui();
	int hide_gui();
	int set_string();
	int load_defaults();
	int save_defaults();
	int save_data(char *text);
	int read_data(char *text);

// parameters needed for oil painting
	int reconfigure();
	int oil_rgb(VPixel **in_rows, VPixel **out_rows, int use_intensity);

	int radius;
	int use_intensity;
    int redo_buffers;

// a thread for the GUI
	OilThread *thread;

private:
	BC_Hash *defaults;
	VFrame *temp_frame;
};


#endif
