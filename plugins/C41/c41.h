/*
 * C41 plugin for Cinelerra
 * Copyright (C) 2011 Florent Delannoy <florent at plui dot es>
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

#ifndef C41_H
#define C41_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("C41")
#define PLUGIN_CLASS C41Effect
#define PLUGIN_CONFIG_CLASS C41Config
#define PLUGIN_THREAD_CLASS C41Thread
#define PLUGIN_GUI_CLASS C41Window

#define SLIDER_LENGTH 1000

#include "pluginmacros.h"

#include "bcslider.h"
#include "bctoggle.h"
#include "keyframe.inc"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

// C41_FAST_POW increases processing speed more than 10 times
// Depending on gcc version, used optimizations and cpu C41_FAST_POW may not work
// With gcc versiion >= 4.4 it seems safe to enable C41_FAST_POW
// Test some samples after you have enabled it
#define C41_FAST_POW

#if defined(C41_FAST_POW)
#define C41_POW_FUNC myPow
#else
#define C41_POW_FUNC powf
#endif

// Shave the image in order to avoid black borders
// The min max pixel value difference must be at least 0.05
#define C41_SHAVE_TOLERANCE 0.05
// Shave some amount - blurring at the edges is not good
#define C41_SHAVE_BLUR 0.20

#include <stdint.h>
#include <string.h>

struct magic
{
	float min_r;
	float min_g;
	float min_b;
	float light;
	float gamma_g;
	float gamma_b;
	float coef1;
	float coef2;
	int shave_min_row;
	int shave_max_row;
	int shave_min_col;
	int shave_max_col;
	int frame_max_col;
	int frame_max_row;
};

class C41Config
{
public:
	C41Config();

	void copy_from(C41Config &src);
	int equivalent(C41Config &src);
	void interpolate(C41Config &prev,
			C41Config &next,
			ptstime prev_pts,
			ptstime next_pts,
			ptstime current_pts);

	int active;
	int compute_magic;
	int postproc;
	int show_box;
	float fix_min_r;
	float fix_min_g;
	float fix_min_b;
	float fix_light;
	float fix_gamma_g;
	float fix_gamma_b;
	float fix_coef1;
	float fix_coef2;
	int min_col;
	int max_col;
	int min_row;
	int max_row;
	int frame_max_row;
	int frame_max_col;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class C41Enable : public BC_CheckBox
{
public:
	C41Enable(C41Effect *plugin, int *output, int x, int y, const char *text);
	int handle_event();

	C41Effect *plugin;
	int *output;
};

class C41TextBox : public BC_TextBox
{
public:
	C41TextBox(C41Effect *plugin, float *value, int x, int y);
	int handle_event();

	C41Effect *plugin;
	float *boxValue;
};

class C41Button : public BC_GenericButton
{
public:
	C41Button(C41Effect *plugin, C41Window *window, int x, int y);
	int handle_event();

	C41Effect *plugin;
	C41Window *window;
};

class C41BoxButton : public BC_GenericButton
{
public:
	C41BoxButton(C41Effect *plugin, C41Window *window, int x, int y);
	int handle_event();

	C41Effect *plugin;
	C41Window *window;
};

class C41Slider : public BC_ISlider
{
public:
	C41Slider(C41Effect *plugin, int *output, int x, int y, int max);

	int handle_event();

	C41Effect *plugin;
	int *output;
};

class C41Window : public PluginWindow
{
public:
	C41Window(C41Effect *plugin, int x, int y);

	void update();
	void update_magic();

	C41Enable *active;
	C41Enable *compute_magic;
	C41Enable *postproc;
	C41Enable *show_box;
	BC_Title *min_r;
	BC_Title *min_g;
	BC_Title *min_b;
	BC_Title *light;
	BC_Title *gamma_g;
	BC_Title *gamma_b;
	BC_Title *coef1;
	BC_Title *coef2;
	BC_Title *box_col_min;
	BC_Title *box_col_max;
	BC_Title *box_row_min;
	BC_Title *box_row_max;
	C41TextBox *fix_min_r;
	C41TextBox *fix_min_g;
	C41TextBox *fix_min_b;
	C41TextBox *fix_light;
	C41TextBox *fix_gamma_g;
	C41TextBox *fix_gamma_b;
	C41TextBox *fix_coef1;
	C41TextBox *fix_coef2;
	C41Button *lock;
	C41BoxButton *boxlock;
	C41Slider *min_row;
	C41Slider *max_row;
	C41Slider *min_col;
	C41Slider *max_col;

	int slider_max_col;
	int slider_max_row;

	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class C41Effect : public PluginVClient
{
public:
	C41Effect(PluginServer *server);
	~C41Effect();

	VFrame *process_tmpframe(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void render_gui(void* data);
	float fix_exepts(float ival);
	float normalize_pixel(float ival);
	uint16_t pixtouint(float val);
#if defined(C41_FAST_POW)
	float myLog2(float i) __attribute__ ((optimize(0)));
	float myPow2(float i) __attribute__ ((optimize(0)));
	float myPow(float a, float b);
#endif
	VFrame* tmp_frame;
	VFrame* blurry_frame;
	struct magic values;

	float *pv_min;
	float *pv_max;
	int pv_alloc;

	int shave_min_row;
	int shave_max_row;
	int shave_min_col;
	int shave_max_col;
	int min_col;
	int max_col;
	int min_row;
	int max_row;

	PLUGIN_CLASS_MEMBERS
};

#endif
