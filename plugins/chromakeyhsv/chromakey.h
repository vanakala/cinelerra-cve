
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

#ifndef CHROMAKEY_H
#define CHROMAKEY_H




#include "colorpicker.h"
#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"


class ChromaKeyHSV;
class ChromaKeyHSV;
class ChromaKeyWindow;

enum {
	CHROMAKEY_POSTPROCESS_NONE,
	CHROMAKEY_POSTPROCESS_BLUR,
	CHROMAKEY_POSTPROCESS_DILATE
};

class ChromaKeyConfig
{
public:
	ChromaKeyConfig();

	void copy_from(ChromaKeyConfig &src);
	int equivalent(ChromaKeyConfig &src);
	void interpolate(ChromaKeyConfig &prev, 
		ChromaKeyConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	int get_color();

	// Output mode
	bool  show_mask;
	// Key color definition
	float red;
	float green;
	float blue;
	// Key shade definition
	float min_brightness;
	float max_brightness;
	float saturation;
	float min_saturation;
	float tolerance;
	// Mask feathering
	float in_slope;
	float out_slope;
	float alpha_offset;
	// Spill light compensation
	float spill_threshold;
	float spill_amount;
};

class ChromaKeyColor : public BC_GenericButton
{
public:
	ChromaKeyColor(ChromaKeyHSV *plugin, 
		ChromaKeyWindow *gui, 
		int x, 
		int y);

	int handle_event();

	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyMinBrightness : public BC_FSlider
{
	public:
		ChromaKeyMinBrightness(ChromaKeyHSV *plugin, int x, int y);
		int handle_event();
		ChromaKeyHSV *plugin;
};

class ChromaKeyMaxBrightness : public BC_FSlider
{
	public:
		ChromaKeyMaxBrightness(ChromaKeyHSV *plugin, int x, int y);
		int handle_event();
		ChromaKeyHSV *plugin;
};

class ChromaKeySaturation : public BC_FSlider
{
	public:
		ChromaKeySaturation(ChromaKeyHSV *plugin, int x, int y);
		int handle_event();
		ChromaKeyHSV *plugin;
};

class ChromaKeyMinSaturation : public BC_FSlider
{
	public:
		ChromaKeyMinSaturation(ChromaKeyHSV *plugin, int x, int y);
		int handle_event();
		ChromaKeyHSV *plugin;
};



class ChromaKeyTolerance : public BC_FSlider
{
public:
	ChromaKeyTolerance(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyInSlope : public BC_FSlider
{
	public:
		ChromaKeyInSlope(ChromaKeyHSV *plugin, int x, int y);
		int handle_event();
		ChromaKeyHSV *plugin;
};

class ChromaKeyOutSlope : public BC_FSlider
{
	public:
		ChromaKeyOutSlope(ChromaKeyHSV *plugin, int x, int y);
		int handle_event();
		ChromaKeyHSV *plugin;
};

class ChromaKeyAlphaOffset : public BC_FSlider
{
	public:
		ChromaKeyAlphaOffset(ChromaKeyHSV *plugin, int x, int y);
		int handle_event();
		ChromaKeyHSV *plugin;
};

class ChromaKeySpillThreshold : public BC_FSlider
{
public:
	ChromaKeySpillThreshold(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};
class ChromaKeySpillAmount : public BC_FSlider
{
public:
	ChromaKeySpillAmount(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};

class ChromaKeyUseColorPicker : public BC_GenericButton
{
public:
	ChromaKeyUseColorPicker(ChromaKeyHSV *plugin, ChromaKeyWindow *gui, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyColorThread : public ColorThread
{
public:
	ChromaKeyColorThread(ChromaKeyHSV *plugin, ChromaKeyWindow *gui);
	int handle_new_color(int output, int alpha);
	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeyShowMask : public BC_CheckBox
{
public:
	ChromaKeyShowMask(ChromaKeyHSV *plugin, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
};



class ChromaKeyWindow : public BC_Window
{
public:
	ChromaKeyWindow(ChromaKeyHSV *plugin, int x, int y);
	~ChromaKeyWindow();

	void create_objects();
	int close_event();
	void update_sample();

	ChromaKeyColor *color;
	ChromaKeyUseColorPicker *use_colorpicker;
	ChromaKeyMinBrightness *min_brightness;
	ChromaKeyMaxBrightness *max_brightness;
	ChromaKeySaturation *saturation;
	ChromaKeyMinSaturation *min_saturation;
	ChromaKeyTolerance *tolerance;
	ChromaKeyInSlope *in_slope;
	ChromaKeyOutSlope *out_slope;
	ChromaKeyAlphaOffset *alpha_offset;
	ChromaKeySpillThreshold *spill_threshold;
	ChromaKeySpillAmount *spill_amount;
	ChromaKeyShowMask *show_mask;
	BC_SubWindow *sample;
	ChromaKeyHSV *plugin;
	ChromaKeyColorThread *color_thread;
};





PLUGIN_THREAD_HEADER(ChromaKeyHSV, ChromaKeyThread, ChromaKeyWindow)


class ChromaKeyServer : public LoadServer
{
public:
	ChromaKeyServer(ChromaKeyHSV *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ChromaKeyHSV *plugin;
};

class ChromaKeyPackage : public LoadPackage
{
public:
	ChromaKeyPackage();
	int y1, y2;
};

class ChromaKeyUnit : public LoadClient
{
public:
	ChromaKeyUnit(ChromaKeyHSV *plugin, ChromaKeyServer *server);
	void process_package(LoadPackage *package);
	template <typename component_type> void process_chromakey(int components, component_type max, bool use_yuv, ChromaKeyPackage *pkg);
	bool is_same_color(float r, float g, float b, float rk,float bk,float gk, float color_threshold, float light_threshold, int key_main_component);

	ChromaKeyHSV *plugin;

};




class ChromaKeyHSV : public PluginVClient
{
public:
	ChromaKeyHSV(PluginServer *server);
	~ChromaKeyHSV();
	
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int handle_opengl();
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_configuration();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int show_gui();
	int set_string();
	void raise_window();
	void update_gui();

	ChromaKeyConfig config;
	VFrame *input, *output;
	ChromaKeyServer *engine;
	ChromaKeyThread *thread;
	BC_Hash *defaults;
};








#endif







