// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

	int is_video() { return 1; };

// User calls this to request an opengl routine to be run synchronously.
	void run_opengl() {};

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
