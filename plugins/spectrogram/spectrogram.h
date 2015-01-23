
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

#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#define PLUGIN_TITLE N_("Spectrogram")

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_CLASS Spectrogram
#define PLUGIN_CONFIG_CLASS SpectrogramConfig
#define PLUGIN_THREAD_CLASS SpectrogramThread
#define PLUGIN_GUI_CLASS SpectrogramWindow

#include "pluginmacros.h"
#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
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


class SpectrogramWindow : public PluginWindow
{
public:
	SpectrogramWindow(Spectrogram *plugin, int x, int y);
	~SpectrogramWindow();

	void update();

	SpectrogramLevel *level;
	int done;
	BC_SubWindow *canvas;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class SpectrogramFFT : public CrossfadeFFT
{
public:
	SpectrogramFFT(Spectrogram *plugin, int window_size);
	~SpectrogramFFT();

	void signal_process();
	void get_frame(AFrame *aframe);

	Spectrogram *plugin;
};


class SpectrogramConfig
{
public:
	SpectrogramConfig();
	double level;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class Spectrogram : public PluginAClient
{
public:
	Spectrogram(PluginServer *server);
	~Spectrogram();

	PLUGIN_CLASS_MEMBERS;

	void process_frame(AFrame *aframe);
	void load_defaults();
	void save_defaults();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void render_gui(void *data, int size);

	void reset();

	int done;

	int need_reconfigure;
	SpectrogramFFT *fft;
	float *data;
	int total_windows;
};

#endif
