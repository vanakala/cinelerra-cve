// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef REVERSEAUDIO_H
#define REVERSEAUDIO_H

// Old name was "Reverse audio"
#define PLUGIN_TITLE N_("Reverse")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CLASS ReverseAudio

#include "pluginmacros.h"

#include "aframe.inc"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"


class ReverseAudio : public PluginAClient
{
public:
	ReverseAudio(PluginServer *server);
	~ReverseAudio();

	PLUGIN_CLASS_MEMBERS

	AFrame *process_tmpframe(AFrame *);

	ptstime input_pts;
	int fragment_size;
};

#endif
