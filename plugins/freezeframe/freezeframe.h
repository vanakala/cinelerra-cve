// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FREEZEFRAME_H
#define FREEZEFRAME_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Freeze Frame")
#define PLUGIN_CLASS FreezeFrameMain

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"

class FreezeFrameMain : public PluginVClient
{
public:
	FreezeFrameMain(PluginServer *server);
	~FreezeFrameMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void handle_opengl();

// Frame to replicate
	VFrame *first_frame;
};

#endif
