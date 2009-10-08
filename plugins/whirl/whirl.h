
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

#ifndef WHIRL_H
#define WHIRL_H

#define MAXANGLE 360
#define MAXPINCH 100
#define MAXRADIUS 100

class WhirlMain;
class WhirlEngine;

#include "bcbase.h"
#include "whirlwindow.h"
#include "pluginvclient.h"


class WhirlMain : public PluginVClient
{
public:
	WhirlMain(int argc, char *argv[]);
	~WhirlMain();

// required for all realtime plugins
	int process_realtime(long size, VFrame **input_ptr, VFrame **output_ptr);
	int plugin_is_realtime();
	int plugin_is_multi_channel();
	char* plugin_title();
	int start_realtime();
	int stop_realtime();
	int start_gui();
	int stop_gui();
	int show_gui();
	int hide_gui();
	int set_string();
	int load_defaults();
	int save_defaults();
	int save_data(char *text);
	int read_data(char *text);

// parameters needed
	int reconfigure();    // Rebuild tables
	int angle;
	int pinch;
	int radius;
	int automated_function;
	int reconfigure_flag;
	VFrame *temp_frame;

// a thread for the GUI
	WhirlThread *thread;

private:
	BC_Hash *defaults;
	WhirlEngine **engine;
};

class WhirlEngine : public Thread
{
public:
	WhirlEngine(WhirlMain *plugin, int start_y, int end_y);
	~WhirlEngine();
	
	int start_process_frame(VFrame **output, VFrame **input, int size);
	int wait_process_frame();
	void run();

	int calc_undistorted_coords(double  wx,
			 double wy,
			 double &whirl,
			 double &pinch,
			 double &x,
			 double &y);
	inline VWORD bilinear(double x, double y, VWORD *values)
	{
		double m0, m1;
		x = fmod(x, 1.0);
		y = fmod(y, 1.0);

		if(x < 0.0) x += 1.0;
		if(y < 0.0) y += 1.0;

		m0 = (double)values[0] + x * ((double)values[1] - values[0]);
		m1 = (double)values[2] + x * ((double)values[3] - values[2]);
		return (VWORD)(m0 + y * (m1 - m0));
	}
	void get_pixel(const int &x, const int &y, VPixel *pixel, VPixel **input_rows);

	WhirlMain *plugin;
	int start_y;
	int end_y;
	int size;
	VFrame **output, **input;
	int last_frame;
	Mutex input_lock, output_lock;
	double radius, radius2, radius3, pinch;
	double scale_x, scale_y;
	double cen_x, cen_y;
};


#endif
