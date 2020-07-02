// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RGB601_H
#define RGB601_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("RGB - 601")
#define PLUGIN_CLASS RGB601Main
#define PLUGIN_CONFIG_CLASS RGB601Config
#define PLUGIN_THREAD_CLASS RGB601Thread
#define PLUGIN_GUI_CLASS RGB601Window

#include "pluginmacros.h"

#define TOTAL_PATTERNS 2

#include "language.h"
#include "pluginvclient.h"
#include <sys/types.h>

// direction values
#define RGB601_NONE  0
#define RGB601_FORW  1
#define RGB601_RVRS  2

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

	VFrame *process_tmpframe(VFrame *frame);
	void reset_plugin();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();

	void create_table(VFrame *input_ptr);
	void process(VFrame *input_ptr);

	int *forward_table;
	int *reverse_table;
};

#endif
