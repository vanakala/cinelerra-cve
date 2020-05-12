// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DELAYAUDIO_H
#define DELAYAUDIO_H

// Old title was "Delay audio"
#define PLUGIN_TITLE N_("Delay")

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_CLASS DelayAudio
#define PLUGIN_CONFIG_CLASS DelayAudioConfig
#define PLUGIN_THREAD_CLASS DelayAudioThread
#define PLUGIN_GUI_CLASS DelayAudioWindow
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#include "pluginmacros.h"

#include "bctextbox.h"
#include "bchash.inc"
#include "language.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include "vframe.inc"
#include <vector>

class DelayAudioTextBox;

class DelayAudioConfig
{
public:
	DelayAudioConfig();

	PLUGIN_CONFIG_CLASS_MEMBERS

	ptstime length;
};

PLUGIN_THREAD_HEADER

class DelayAudioWindow : public PluginWindow
{
public:
	DelayAudioWindow(DelayAudio *plugin, int x, int y);

	void update();

	DelayAudioTextBox *length;
	PLUGIN_GUI_CLASS_MEMBERS
};


class DelayAudioTextBox : public BC_TextBox
{
public:
	DelayAudioTextBox(DelayAudio *plugin, int x, int y);

	int handle_event();

	DelayAudio *plugin;
};


class DelayAudio : public PluginAClient
{
public:
	DelayAudio(PluginServer *server);
	~DelayAudio();

	PLUGIN_CLASS_MEMBERS;

	void load_defaults();
	void save_defaults();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	AFrame *process_tmpframe(AFrame *input);

	std::vector<double> buffer;
};

#endif
