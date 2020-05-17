// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef NORMALIZE_H
#define NORMALIZE_H

#define PLUGIN_TITLE N_("Normalize")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CLASS NormalizeMain
#define PLUGIN_GUI_CLASS NormalizeWindow

#include "pluginmacros.h"
#include "cinelerra.h"
#include "pluginaclient.h"


class NormalizeMain : public PluginAClient
{
public:
	NormalizeMain(PluginServer *server);
	~NormalizeMain();

	PLUGIN_CLASS_MEMBERS

// normalizing engine

// parameters needed
	double db_over;
	int separate_tracks;

	int process_loop(AFrame **buffer);

	void load_defaults();
	void save_defaults();

// Current state of process_loop
	int writing;
	double peak[MAXCHANNELS], scale[MAXCHANNELS];
};

#endif
