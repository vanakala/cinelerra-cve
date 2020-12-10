// SPDX-License-Identifier: GPL-2.0-or-later

// This file is part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DENOISEFFT_H
#define DENOISEFFT_H

#define PLUGIN_TITLE N_("DenoiseFFT")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_CLASS DenoiseFFTEffect
#define PLUGIN_CONFIG_CLASS DenoiseFFTConfig
#define PLUGIN_THREAD_CLASS DenoiseFFTThread
#define PLUGIN_GUI_CLASS DenoiseFFTWindow

#include "pluginmacros.h"

#include "aframe.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bctitle.h"
#include "bcpot.h"
#include "fourier.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"

#define WINDOW_SIZE 16384

// Noise collection is done either from the start of the effect or the start
// of the previous keyframe.  It always covers the higher numbered samples
// after the keyframe.

class DenoiseFFTConfig
{
public:
	DenoiseFFTConfig();

	ptstime referencetime;
	double level;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class DenoiseFFTLevel : public BC_FPot
{
public:
	DenoiseFFTLevel(DenoiseFFTEffect *plugin, int x, int y);
	int handle_event();
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTSamples : public BC_PopupMenu
{
public:
	DenoiseFFTSamples(DenoiseFFTEffect *plugin, int x, int y, char *text);
	int handle_event();
	DenoiseFFTEffect *plugin;
};

class DenoiseFFTWindow : public PluginWindow
{
public:
	DenoiseFFTWindow(DenoiseFFTEffect *plugin, int x, int y);

	void update();

	DenoiseFFTLevel *level;
	DenoiseFFTSamples *samples;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class DenoiseFFTRemove : public Fourier
{
public:
	DenoiseFFTRemove(DenoiseFFTEffect *plugin, int window_size);

	int signal_process();

	DenoiseFFTEffect *plugin;
};

class DenoiseFFTCollect : public Fourier
{
public:
	DenoiseFFTCollect(DenoiseFFTEffect *plugin, int window_size);

	int signal_process();

	DenoiseFFTEffect *plugin;
};

class DenoiseFFTEffect : public PluginAClient
{
public:
	DenoiseFFTEffect(PluginServer *server);
	~DenoiseFFTEffect();

	void reset_plugin();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	AFrame *process_tmpframe(AFrame *aframe);
	void collect_noise();

	void load_defaults();
	void save_defaults();

	void process_window();

	PLUGIN_CLASS_MEMBERS

	AFrame *input_frame;
// Start of sample of noise to collect
	ptstime collection_pts;
	double *reference;
	DenoiseFFTRemove *remove_engine;
	DenoiseFFTCollect *collect_engine;
};

#endif
