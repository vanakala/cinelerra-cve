
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef DELAYAUDIO_H
#define DELAYAUDIO_H

// Old title was "Delay audio"
#define PLUGIN_TITLE N_("Delay")

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME

#define PLUGIN_CLASS DelayAudio
#define PLUGIN_CONFIG_CLASS DelayAudioConfig
#define PLUGIN_THREAD_CLASS DelayAudioThread
#define PLUGIN_GUI_CLASS DelayAudioWindow
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#include "pluginmacros.h"
#include "bchash.inc"
#include "language.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
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

class DelayAudioWindow : public BC_Window
{
public:
	DelayAudioWindow(DelayAudio *plugin, int x, int y);
	~DelayAudioWindow();

	void update();

	DelayAudioTextBox *length;
	PLUGIN_GUI_CLASS_MEMBERS
};


class DelayAudioTextBox : public BC_TextBox
{
public:
	DelayAudioTextBox(DelayAudio *plugin, int x, int y);
	~DelayAudioTextBox();

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
	void process_realtime(AFrame *input, AFrame *output);

	std::vector<double> buffer;
};

#endif
