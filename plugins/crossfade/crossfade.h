// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#ifndef CROSSFADE_H
#define CROSSFADE_H

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_TRANSITION
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_TITLE N_("Crossfade")
#define PLUGIN_CLASS CrossfadeMain

#include "language.h"
#include "pluginmacros.h"
#include "pluginaclient.h"

class CrossfadeMain : public PluginAClient
{
public:
	CrossfadeMain(PluginServer *server);
	~CrossfadeMain();

	PLUGIN_CLASS_MEMBERS;

// required for all transition plugins
	void process_realtime(AFrame *input, AFrame *output);
};

#endif
