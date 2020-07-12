// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef REVERSEVIDEO_H
#define REVERSEVIDEO_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

// Old name was "Reverse video"
#define PLUGIN_TITLE N_("Reverse")
#define PLUGIN_CLASS ReverseVideo

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"


class ReverseVideo : public PluginVClient
{
public:
	ReverseVideo(PluginServer *server);
	~ReverseVideo();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
};

#endif
