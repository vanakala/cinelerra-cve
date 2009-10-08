
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

#ifndef RGB601_H
#define RGB601_H

class RGB601Main;

#define TOTAL_PATTERNS 2

#include "bchash.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "rgb601window.h"
#include <sys/types.h>

class RGB601Config
{
public:
	RGB601Config();
// 0 -> none 
// 1 -> RGB -> 601 
// 2 -> 601 -> RGB
	int direction;
};

class RGB601Main : public PluginVClient
{
public:
	RGB601Main(PluginServer *server);
	~RGB601Main();

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	char* plugin_title();
	int show_gui();
	void update_gui();
	void raise_window();
	int set_string();
	void load_configuration();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	VFrame* new_picon();
	int handle_opengl();

	void create_table(VFrame *input_ptr);
	void process(int *table, VFrame *input_ptr, VFrame *output_ptr);
	
	RGB601Thread *thread;
	RGB601Config config;
	BC_Hash *defaults;
	int forward_table[0x10000], reverse_table[0x10000];
};


#endif
