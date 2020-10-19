// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#define PLUGIN_TITLE N_("Spectrogram")

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_NOT_KEYFRAMEABLE
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_CLASS Spectrogram
#define PLUGIN_CONFIG_CLASS SpectrogramConfig
#define PLUGIN_THREAD_CLASS SpectrogramThread
#define PLUGIN_GUI_CLASS SpectrogramWindow

#include "pluginmacros.h"

#include "bcpot.h"
#include "bctoggle.h"
#include "fourier.h"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class SpectrogramFFT;


class SpectrogramLevel : public BC_FPot
{
public:
	SpectrogramLevel(Spectrogram *plugin, int x, int y);

	int handle_event();
	Spectrogram *plugin;
};

class SpectrogramBW : public BC_CheckBox
{
public:
	SpectrogramBW(Spectrogram *plugin, int x, int y);

	int handle_event();

	Spectrogram *plugin;
};

class SpectrogramWindow : public PluginWindow
{
public:
	SpectrogramWindow(Spectrogram *plugin, int x, int y);
	~SpectrogramWindow();

	void update();
	void update_canvas();

	SpectrogramLevel *level;
	SpectrogramBW *blackwhite;
	BC_SubWindow *canvas;
	int gui_tmp_size;
	double *gui_tmp;

	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class SpectrogramFFT : public Fourier
{
public:
	SpectrogramFFT(Spectrogram *plugin, int window_size);

	int signal_process();

	Spectrogram *plugin;
};


class SpectrogramConfig
{
public:
	SpectrogramConfig();

	double level;
	int blackwhite;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class Spectrogram : public PluginAClient
{
public:
	Spectrogram(PluginServer *server);
	~Spectrogram();

	PLUGIN_CLASS_MEMBERS;

	void reset_plugin();
	AFrame *process_tmpframe(AFrame *aframe);
	void load_defaults();
	void save_defaults();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);

	int window_size;
	int data_size;
	int window_num;
	AFrame *frame;
	double *data;

private:
	SpectrogramFFT *fft;
};

#endif
