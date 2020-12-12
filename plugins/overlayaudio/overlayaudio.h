// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef OVERLAYAUDIO_H
#define OVERLAYAUDIO_H

#define PLUGIN_TITLE N_("Overlay")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_CLASS OverlayAudio

#include "pluginmacros.h"

#include "aframe.inc"
#include "language.h"
#include "pluginaclient.h"


class OverlayAudio : public PluginAClient
{
public:
	OverlayAudio(PluginServer *server);
	~OverlayAudio();

	void process_tmpframes(AFrame **aframes);

	PLUGIN_CLASS_MEMBERS
};

#endif
