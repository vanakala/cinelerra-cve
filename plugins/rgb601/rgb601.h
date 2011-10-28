
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("RGB - 601")
#define PLUGIN_CLASS RGB601Main
#define PLUGIN_CONFIG_CLASS RGB601Config
#define PLUGIN_THREAD_CLASS RGB601Thread
#define PLUGIN_GUI_CLASS RGB601Window

#include "pluginmacros.h"

#define TOTAL_PATTERNS 2

#include "bchash.h"
#include "language.h"
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
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class RGB601Main : public PluginVClient
{
public:
	RGB601Main(PluginServer *server);
	~RGB601Main();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_frame(VFrame *frame);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();

	void create_table(VFrame *input_ptr);
	void process(int *table, VFrame *input_ptr, VFrame *output_ptr);

	int forward_table[0x10000], reverse_table[0x10000];
};


#endif
