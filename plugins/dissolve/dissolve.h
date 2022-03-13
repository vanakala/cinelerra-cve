// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DISSOLVE_H
#define DISSOLVE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Dissolve")
#define PLUGIN_CLASS DissolveMain

#include "pluginmacros.h"

class DissolveMain;

#include "language.h"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

class DissolveMain : public PluginVClient
{
public:
	DissolveMain(PluginServer *server);
	~DissolveMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	void reset_plugin();
	void handle_opengl();

	OverlayFrame *overlayer;
};

#endif
