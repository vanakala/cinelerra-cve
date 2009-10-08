
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

#ifndef TRANSLATE_H
#define TRANSLATE_H

// the simplest plugin possible

class TranslateMain;

#include "bchash.h"
#include "mutex.h"
#include "translatewin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

class TranslateConfig
{
public:
	TranslateConfig();
	int equivalent(TranslateConfig &that);
	void copy_from(TranslateConfig &that);
	void interpolate(TranslateConfig &prev, 
		TranslateConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	float in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h;
};


class TranslateMain : public PluginVClient
{
public:
	TranslateMain(PluginServer *server);
	~TranslateMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void raise_window();
	void update_gui();
	int set_string();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	VFrame* new_picon();
	int load_defaults();
	int save_defaults();
	int load_configuration();

// a thread for the GUI
	TranslateThread *thread;

	OverlayFrame *overlayer;   // To translate images
	VFrame *temp_frame;        // Used if buffers are the same
	BC_Hash *defaults;
	TranslateConfig config;
};


#endif
