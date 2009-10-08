
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

#ifndef THRESHOLD_H
#define THRESHOLD_H


#include "histogramengine.inc"
#include "loadbalance.h"
#include "thresholdwindow.inc"
#include "plugincolors.inc"
#include "pluginvclient.h"


class ThresholdEngine;
class RGBA;

// Color components are in range [0, 255].
class RGBA
{
 public:
	RGBA();   // Initialize to transparent black.
	RGBA(int r, int g, int b, int a);
	void set(int r, int g, int b, int a);
	void set(int rgb, int alpha);  // red in byte 2, green in byte 1, blue in byte 0.
	int getRGB() const; // Encode red in byte 2, green in byte 1, blue in byte 0.

	// Load values in BC_Hash and return in an RGBA.
	// Use values in this RGBA as defaults.
	RGBA load_default(BC_Hash * defaults, const char * prefix) const;

	// Save values in this RGBA to the BC_Hash.
	void save_defaults(BC_Hash * defaults, const char * prefix) const;

	// Set R, G, B, A properties from this RGBA.
	void set_property(XMLTag & tag, const char * prefix) const;

	// Load R, G, B, A properties and return in an RGBA.
	// Use values in this RGBA as defaults.
	RGBA get_property(XMLTag & tag, const char * prefix) const;

	int  r, g, b, a;
};

bool operator==(const RGBA & a, const RGBA & b);

// General purpose scale function.
template<typename T>
T interpolate(const T & prev, const double & prev_scale, const T & next, const double & next_scale);

// Specialization for RGBA class.
template<>
RGBA interpolate(const RGBA & prev_color, const double & prev_scale, const RGBA &next_color, const double & next_scale);



class ThresholdConfig
{
public:
	ThresholdConfig();
	int equivalent(ThresholdConfig &that);
	void copy_from(ThresholdConfig &that);
	void interpolate(ThresholdConfig &prev,
		ThresholdConfig &next,
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void reset();
	void boundaries();

	float min;
	float max;
	int plot;
	RGBA low_color;
	RGBA mid_color;
	RGBA high_color;
};



class ThresholdMain : public PluginVClient
{
public:
	ThresholdMain(PluginServer *server);
	~ThresholdMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data);
	void calculate_histogram(VFrame *frame);
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(ThresholdConfig, ThresholdThread);
	HistogramEngine *engine;
	ThresholdEngine *threshold_engine;
};






class ThresholdPackage : public LoadPackage
{
public:
	ThresholdPackage();
	int start;
	int end;
};


class ThresholdUnit : public LoadClient
{
public:
	ThresholdUnit(ThresholdEngine *server);
	void process_package(LoadPackage *package);
	
	ThresholdEngine *server;
private:
	template<typename TYPE, int COMPONENTS, bool USE_YUV>
	void render_data(LoadPackage *package);
};


class ThresholdEngine : public LoadServer
{
public:
	ThresholdEngine(ThresholdMain *plugin);
	~ThresholdEngine();

	void process_packages(VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	YUV *yuv;
	ThresholdMain *plugin;
	VFrame *data;
};






#endif
