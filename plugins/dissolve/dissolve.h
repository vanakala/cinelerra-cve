
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

#ifndef DISSOLVE_H
#define DISSOLVE_H

class DissolveMain;

#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

class DissolveMain : public PluginVClient
{
public:
	DissolveMain(PluginServer *server);
	~DissolveMain();

// required for all realtime plugins
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int handle_opengl();
	int uses_gui();
	int is_transition();
	int is_video();
	const char* plugin_title();
	VFrame* new_picon();
	OverlayFrame *overlayer;
	float fade;
};

#endif
