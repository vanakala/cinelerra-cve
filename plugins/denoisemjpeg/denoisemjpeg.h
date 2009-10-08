
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

#ifndef DENOISEMJPEG_H
#define DENOISEMJPEG_H

class DenoiseMJPEG;

#include "bcdisplayinfo.h"
#include "bchash.inc"
#include "pluginvclient.h"
#include "vframe.inc"



class DenoiseMJPEGConfig
{
public:
	DenoiseMJPEGConfig();

	int equivalent(DenoiseMJPEGConfig &that);
	void copy_from(DenoiseMJPEGConfig &that);
	void interpolate(DenoiseMJPEGConfig &prev, 
		DenoiseMJPEGConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	int radius;
	int threshold;
	int threshold2;
	int sharpness;
	int lcontrast;
	int ccontrast;
	int deinterlace;
	int mode;
	int delay;
};


class DenoiseMJPEGWindow;

class DenoiseMJPEGRadius : public BC_IPot
{
public:
	DenoiseMJPEGRadius(DenoiseMJPEG *plugin, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
};

class DenoiseMJPEGThresh : public BC_IPot
{
public:
	DenoiseMJPEGThresh(DenoiseMJPEG *plugin, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
};

class DenoiseMJPEGThresh2 : public BC_IPot
{
public:
	DenoiseMJPEGThresh2(DenoiseMJPEG *plugin, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
};

class DenoiseMJPEGSharp : public BC_IPot
{
public:
	DenoiseMJPEGSharp(DenoiseMJPEG *plugin, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
};

class DenoiseMJPEGLContrast : public BC_IPot
{
public:
	DenoiseMJPEGLContrast(DenoiseMJPEG *plugin, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
};

class DenoiseMJPEGCContrast : public BC_IPot
{
public:
	DenoiseMJPEGCContrast(DenoiseMJPEG *plugin, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
};

class DenoiseMJPEGDeinterlace : public BC_CheckBox
{
public:
	DenoiseMJPEGDeinterlace(DenoiseMJPEG *plugin, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
};

class DenoiseMJPEGModeInterlaced : public BC_Radial
{
public:
	DenoiseMJPEGModeInterlaced(DenoiseMJPEG *plugin, DenoiseMJPEGWindow *gui, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
	DenoiseMJPEGWindow *gui;
};

class DenoiseMJPEGModeProgressive : public BC_Radial
{
public:
	DenoiseMJPEGModeProgressive(DenoiseMJPEG *plugin, DenoiseMJPEGWindow *gui, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
	DenoiseMJPEGWindow *gui;
};

class DenoiseMJPEGModeFast : public BC_Radial
{
public:
	DenoiseMJPEGModeFast(DenoiseMJPEG *plugin, DenoiseMJPEGWindow *gui, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
	DenoiseMJPEGWindow *gui;
};

class DenoiseMJPEGDelay : public BC_IPot
{
public:
	DenoiseMJPEGDelay(DenoiseMJPEG *plugin, int x, int y);
	int handle_event();
	DenoiseMJPEG *plugin;
};


class DenoiseMJPEGWindow : public BC_Window
{
public:
	DenoiseMJPEGWindow(DenoiseMJPEG *plugin, int x, int y);

	void create_objects();
	int close_event();
	void update_mode(int value);

	DenoiseMJPEG *plugin;
	DenoiseMJPEGRadius *radius;
	DenoiseMJPEGThresh *threshold1;
	DenoiseMJPEGThresh2 *threshold2;
	DenoiseMJPEGSharp *sharpness;
	DenoiseMJPEGLContrast *lcontrast;
	DenoiseMJPEGCContrast *ccontrast;
	DenoiseMJPEGDeinterlace *deinterlace;
	DenoiseMJPEGModeInterlaced *interlaced;
	DenoiseMJPEGModeProgressive *progressive;
	DenoiseMJPEGModeFast *fast;
	DenoiseMJPEGDelay *delay;
};





PLUGIN_THREAD_HEADER(DenoiseMJPEG, DenoiseMJPEGThread, DenoiseMJPEGWindow)

class DenoiseMJPEG : public PluginVClient
{
public:
	DenoiseMJPEG(PluginServer *server);
	~DenoiseMJPEG();

	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	int load_configuration();
	int set_string();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void raise_window();
	void update_gui();

	float *accumulation;
	DenoiseMJPEGThread *thread;
	DenoiseMJPEGConfig config;
	BC_Hash *defaults;
};



#endif
