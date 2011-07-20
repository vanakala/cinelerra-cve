
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

#ifndef PARAMETRIC_H
#define PARAMETRIC_H


#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"


// This parametric EQ multiplies the data by a gaussian curve in frequency domain.
// It causes significant delay but is useful.


#define BANDS 3
#define WINDOW_SIZE 16384
#define MAXMAGNITUDE 15

class ParametricConfig;
class ParametricThread;
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
	float quality;
	float magnitude;
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
	float wetness;
};


PLUGIN_THREAD_HEADER(ParametricEQ, ParametricThread, ParametricWindow)


class ParametricFreq : public BC_QPot
{
public:
	ParametricFreq(ParametricEQ *plugin, int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
};


class ParametricQuality : public BC_FPot
{
public:
	ParametricQuality(ParametricEQ *plugin, int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
};


class ParametricMagnitude : public BC_FPot
{
public:
	ParametricMagnitude(ParametricEQ *plugin, int x, int y, int band);

	int handle_event();

	int band;
	ParametricEQ *plugin;
};


class ParametricMode : public BC_PopupMenu
{
public:
	ParametricMode(ParametricEQ *plugin, int x, int y, int band);

	void create_objects();
	int handle_event();
	static int text_to_mode(const char *text);
	static const char* mode_to_text(int mode);

	int band;
	ParametricEQ *plugin;
};


class ParametricBandGUI
{
public:
	ParametricBandGUI(ParametricEQ *plugin, 
		ParametricWindow *window, 
		int x, 
		int y, 
		int band);
	~ParametricBandGUI();

	void create_objects();
	void update_gui();

	int band;
	int x, y;
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
	ParametricWetness(ParametricEQ *plugin, int x, int y);
	int handle_event();
	ParametricEQ *plugin;
};


class ParametricWindow : public BC_Window
{
public:
	ParametricWindow(ParametricEQ *plugin, int x, int y);
	~ParametricWindow();

	void create_objects();
	void update_gui();
	void update_canvas();

	BC_SubWindow *canvas;
	ParametricEQ *plugin;
	ParametricBandGUI* bands[BANDS];
	ParametricWetness *wetness;
};


class ParametricFFT : public CrossfadeFFT
{
public:
	ParametricFFT(ParametricEQ *plugin, int window_size);
	~ParametricFFT();

	void signal_process();
	void get_frame(AFrame *aframe);

	ParametricEQ *plugin;
};


class ParametricEQ : public PluginAClient
{
public:
	ParametricEQ(PluginServer *server);
	~ParametricEQ();

	int is_realtime();
	int has_pts_api();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_frame(AFrame *aframe);

	void load_defaults();
	void save_defaults();
	void reconfigure();
	void update_gui();

	void calculate_envelope();
	double gauss(double sigma, double a, double x);

	double envelope[WINDOW_SIZE / 2];
	int need_reconfigure;
	PLUGIN_CLASS_MEMBERS(ParametricConfig, ParametricThread)
	ParametricFFT *fft;
};

#endif
