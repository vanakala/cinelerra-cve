// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LOOPAUDIO_H
#define LOOPAUDIO_H

// Old name was 'Loop audio'
#define PLUGIN_TITLE N_("Loop")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_NOT_KEYFRAMEABLE

#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_CLASS LoopAudio
#define PLUGIN_CONFIG_CLASS LoopAudioConfig
#define PLUGIN_THREAD_CLASS LoopAudioThread
#define PLUGIN_GUI_CLASS LoopAudioWindow

#include "pluginmacros.h"

#include "aframe.inc"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"

class LoopAudioConfig
{
public:
	LoopAudioConfig();

	ptstime duration;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class LoopAudioDuration : public BC_TextBox
{
public:
	LoopAudioDuration(LoopAudio *plugin,
		int x,
		int y);
	int handle_event();

	LoopAudio *plugin;
};

class LoopAudioWindow : public PluginWindow
{
public:
	LoopAudioWindow(LoopAudio *plugin, int x, int y);

	void update();

	LoopAudioDuration *duration;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class LoopAudio : public PluginAClient
{
public:
	LoopAudio(PluginServer *server);
	~LoopAudio();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	AFrame *process_tmpframe(AFrame *aframe);
};

#endif
