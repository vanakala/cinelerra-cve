
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

#ifndef PLUGINVCLIENT_H
#define PLUGINVCLIENT_H

#include "arraylist.h"
#include "bcfontentry.inc"
#include "guidelines.inc"
#include "pluginclient.h"

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

// Maximum dimensions for a temporary frame a plugin should retain between 
// process_buffer calls.  This allows memory conservation.
#define PLUGIN_MAX_W 2000
#define PLUGIN_MAX_H 1000

class PluginVClient : public PluginClient
{
public:
	PluginVClient(PluginServer *server);
	virtual ~PluginVClient() {};

	int is_video();

// User calls this to request an opengl routine to be run synchronously.
	void run_opengl();

// Called by Playback3D to run opengl commands synchronously.
// Overridden by the user with the commands to run synchronously.
	virtual void handle_opengl() {};

// Get list of system fonts
	ArrayList<BC_FontEntry*> *get_fontlist();
// Find font entry
	BC_FontEntry *find_fontentry(const char *displayname, int style,
		int mask, int preferred_style);
// Find font path where glyph for the character_code exists
	int find_font_by_char(FT_ULong char_code, char *path_new, const FT_Face oldface);
// Get guideframe of the plugin
	GuideFrame *get_guides();
};

#endif
