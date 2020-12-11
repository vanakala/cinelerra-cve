// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef INVERT_H
#define INVERT_H

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

// Original name was "Invert Audio"
#define PLUGIN_TITLE N_("Invert")
#define PLUGIN_CLASS InvertAudioEffect

#include "pluginmacros.h"

#include "aframe.inc"
#include "language.h"
#include "pluginaclient.h"
#include "pluginserver.inc"


class InvertAudioEffect : public PluginAClient
{
public:
	InvertAudioEffect(PluginServer *server);
	~InvertAudioEffect();

	PLUGIN_CLASS_MEMBERS

	AFrame *process_tmpframe(AFrame *input);
};

#endif
