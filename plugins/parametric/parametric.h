// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PARAMETRIC_H
#define PARAMETRIC_H

#define PLUGIN_TITLE N_("EQ Parametric")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CLASS ParametricEQ
#define PLUGIN_CONFIG_CLASS ParametricConfig
#define PLUGIN_THREAD_CLASS ParametricThread
#define PLUGIN_GUI_CLASS ParametricWindow

#include "pluginmacros.h"

#include "bchash.inc"
#include "bcpopupmenu.h"
#include "bcpot.h"
#include "bctextbox.h"
#include "fourier.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"

// This parametric EQ multiplies the data by a gaussian curve in frequency domain.
// It causes significant delay but is useful.

#define BANDS 3
#define MAXMAGNITUDE 15

class ParametricFreq;
class ParametricQuality;
class ParametricMagnitude;
class ParametricBandGUI;
class ParametricWindow;
class ParametricFFT;
class ParametricEQ;


class ParametricBand
{
public:
	ParametricBand();

	int equivalent(ParametricBand &that);
	void copy_from(ParametricBand &that);
	void interpolate(ParametricBand &prev, 
		ParametricBand &next, 
		double prev_scale, 
		double next_scale);

	enum
	{
		NONE,
		LOWPASS,
		HIGHPASS,
		BANDPASS
	};

	int freq;
	double quality;
	double magnitude;
	int mode;
};


class ParametricConfig
{
public:
	ParametricConfig();

	int equivalent(ParametricConfig &that);
	void copy_from(ParametricConfig &that);
	void interpolate(ParametricConfig &prev, 
		ParametricConfig &next, 
		ptstime prev_pts,
		ptstime next_pts, 
		ptstime current_pts);

	ParametricBand band[BANDS];
	double wetness;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class ParametricFreq : public BC_QPot
{
public:
	ParametricFreq(ParametricEQ *plugin, ParametricWindow *window,
		int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
	ParametricWindow *window;
};


class ParametricQuality : public BC_FPot
{
public:
	ParametricQuality(ParametricEQ *plugin, ParametricWindow *window,
		int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
	ParametricWindow *window;
};


class ParametricMagnitude : public BC_FPot
{
public:
	ParametricMagnitude(ParametricEQ *plugin, ParametricWindow *window,
		int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
	ParametricWindow *window;
};


class ParametricMode : public BC_PopupMenu
{
public:
	ParametricMode(ParametricEQ *plugin, ParametricWindow *window,
		int x, int y, int band);

	int handle_event();
	static int text_to_mode(const char *text);
	static const char* mode_to_text(int mode);
	void update(int mode);

	int band;
	ParametricEQ *plugin;
	ParametricWindow *window;
};


class ParametricBandGUI
{
public:
	ParametricBandGUI(ParametricEQ *plugin, 
		ParametricWindow *window, 
		int x, 
		int y, 
		int band);

	void update_gui();

	int band;
	ParametricEQ *plugin;
	ParametricWindow *window;
	ParametricFreq *freq;
	ParametricQuality *quality;
	ParametricMagnitude *magnitude;
	ParametricMode *mode;
};


class ParametricWetness : public BC_FPot
{
public:
	ParametricWetness(ParametricEQ *plugin, ParametricWindow *window,
		int x, int y);

	int handle_event();

	ParametricEQ *plugin;
	ParametricWindow *window;
};


class ParametricWindow : public PluginWindow
{
public:
	ParametricWindow(ParametricEQ *plugin, int x, int y);
	~ParametricWindow();

	void update();
	void update_canvas();

	double *window_envelope;
	BC_SubWindow *canvas;
	ParametricBandGUI* bands[BANDS];
	ParametricWetness *wetness;
	PLUGIN_GUI_CLASS_MEMBERS
};


class ParametricFFT : public Fourier
{
public:
	ParametricFFT(ParametricEQ *plugin, int window_size);

	int signal_process();

	ParametricEQ *plugin;
};


class ParametricEQ : public PluginAClient
{
public:
	ParametricEQ(PluginServer *server);
	~ParametricEQ();

	void reset_plugin();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	AFrame *process_tmpframe(AFrame *aframe);

	void load_defaults();
	void save_defaults();

	double *calculate_envelope(double *envelope);
	double gauss(double sigma, double a, double x);

	int niquist;
	double *plugin_envelope;
	PLUGIN_CLASS_MEMBERS
	ParametricFFT *fft;
};

#endif
