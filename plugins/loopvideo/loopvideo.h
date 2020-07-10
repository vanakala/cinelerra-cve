// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LOOPVIDEO_H
#define LOOPVIDEO_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_NOT_KEYFRAMEABLE

// Old name was "Loop video"
#define PLUGIN_TITLE N_("Loop")
#define PLUGIN_CLASS LoopVideo
#define PLUGIN_CONFIG_CLASS LoopVideoConfig
#define PLUGIN_THREAD_CLASS LoopVideoThread
#define PLUGIN_GUI_CLASS LoopVideoWindow

#include "pluginmacros.h"

#include "bctextbox.h"
#include "keyframe.inc"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class LoopVideo;

class LoopVideoConfig
{
public:
	LoopVideoConfig();

	ptstime duration;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class LoopVideoFrames : public BC_TextBox
{
public:
	LoopVideoFrames(LoopVideo *plugin,
		int x,
		int y);
	int handle_event();
	LoopVideo *plugin;
};

class LoopVideoWindow : public PluginWindow
{
public:
	LoopVideoWindow(LoopVideo *plugin, int x, int y);

	void update();

	LoopVideoFrames *frames;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class LoopVideo : public PluginVClient
{
public:
	LoopVideo(PluginServer *server);
	~LoopVideo();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	VFrame *process_tmpframe(VFrame *frame);
};

#endif

