
/*
 * Cinelerra :: Blue Banana - color modification plugin for Cinelerra-CV
 * Copyright (C) 2012-2013 Monty <monty@xiph.org>
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

#ifndef BLUEBANANA_H
#define BLUEBANANA_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_STATUS_GUI
#define PLUGIN_TITLE N_("Blue Banana")
#define PLUGIN_CLASS BluebananaMain
#define PLUGIN_CONFIG_CLASS BluebananaConfig
#define PLUGIN_THREAD_CLASS BluebananaThread
#define PLUGIN_GUI_CLASS BluebananaWindow

class BluebananaEngine;
class BluebananaMain;
class BluebananaThread;
#include "pluginmacros.h"

#include "language.h"
#include "bluebananaconfig.h"
#include "bluebananawindow.h"
#include "clip.h"
#include "loadbalance.h"
#include "pluginvclient.h"

#define SELECT_LOOKUP_SIZE 1024
#define ADJUST_LOOKUP_SIZE 4096
#define SELECT_LOOKUP_SCALE 1024.f
#define ADJUST_LOOKUP_SCALE 1024.f
// adjust lookups clamp lookup range / intermediate results above this value
#define MAX_ADJ_HEADROOM 4.

#define MIN_GAMMA .2f
#define MAX_GAMMA 5.f

#define MIN_HISTBRACKET -100
#define MAX_HISTBRACKET 200
#define HISTSIZE 1536
#define HISTSCALE 512
#define HUESCALE  256

// valid HRGBSHIFT is 0 through 3
#define HRGBSHIFT 1
#define HISTOFF 1.f
#define FRAC0   .2f
#define FRAC100 .8f

#define FSrange 15
#define FSovermax 50

#define SELECT_THRESH .00001f

#define HSpV_SATURATION_BIAS (32.f / 255.f)
#define HSpV_SATURATION_SCALE (1.f + HSpV_SATURATION_BIAS)
#define HSpV_SATURATION_ISCALE (1.f / HSpV_SATURATION_SCALE)

#undef CLAMP
#define CLAMP(x, y, z) ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))

class BluebananaMain : public PluginVClient
{
public:
	BluebananaMain(PluginServer *server);
	~BluebananaMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_auto(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void enter_config_change();
	void commit_config_change();
	void leave_config_change();
	void render_gui(void *data);

	float hue_select_alpha(float hue);
	float sat_select_alpha(float sat);
	float val_select_alpha(float val);

	VFrame *frame;
	BluebananaEngine *engine;
	int colormodel;

	friend class BluebananaEngine;

	float hue_select_alpha_lookup[SELECT_LOOKUP_SIZE + 2];
	float sat_select_alpha_lookup[SELECT_LOOKUP_SIZE + 2];
	float val_select_alpha_lookup[SELECT_LOOKUP_SIZE + 2];
	float sat_adj_lookup[ADJUST_LOOKUP_SIZE + 2];
	float sat_adj_toe_slope;
	float val_adj_lookup[ADJUST_LOOKUP_SIZE + 2];
	float val_adj_toe_slope;
	float red_adj_lookup[ADJUST_LOOKUP_SIZE + 2];
	float red_adj_toe_slope;
	float green_adj_lookup[ADJUST_LOOKUP_SIZE + 2];
	float green_adj_toe_slope;
	float blue_adj_lookup[ADJUST_LOOKUP_SIZE + 2];
	float blue_adj_toe_slope;

	float red_histogram[HISTSIZE + (1 << HRGBSHIFT)];
	float green_histogram[HISTSIZE + (1 << HRGBSHIFT)];
	float blue_histogram[HISTSIZE + (1 << HRGBSHIFT)];
	float hue_histogram[HISTSIZE + (1 << HRGBSHIFT)];
	float hue_total;
	float hue_weight;
	float sat_histogram[HISTSIZE + (1 << HRGBSHIFT)];
	float value_histogram[HISTSIZE + (1 << HRGBSHIFT)];

	float hue_histogram_red[(HISTSIZE >> HRGBSHIFT) + 1];
	float hue_histogram_green[(HISTSIZE >> HRGBSHIFT) + 1];
	float hue_histogram_blue[(HISTSIZE >> HRGBSHIFT) + 1];

	float sat_histogram_red[(HISTSIZE >> HRGBSHIFT) + 1];
	float sat_histogram_green[(HISTSIZE >> HRGBSHIFT) + 1];
	float sat_histogram_blue[(HISTSIZE >> HRGBSHIFT) + 1];

	float value_histogram_red[(HISTSIZE >> HRGBSHIFT) + 1];
	float value_histogram_green[(HISTSIZE >> HRGBSHIFT) + 1];
	float value_histogram_blue[(HISTSIZE >> HRGBSHIFT) + 1];

	float *fill_selection(float *in, float *work,
		int width, int height,
		BluebananaEngine *e);

	int select_one_n;
	int select_two_n;
	int select_three_n;
	char select_one[FSrange * 4];
	char select_two[FSrange * 4];
	char select_three[FSrange * 4];

	BluebananaConfig lookup_cache;
	BluebananaConfig update_cache;

	void update_lookups(int serverside);
};

class BluebananaPackage : public LoadPackage
{
public:
	BluebananaPackage(BluebananaEngine *engine);
	BluebananaEngine *engine;

	float Rhist[HISTSIZE + (1 << HRGBSHIFT)];
	float Ghist[HISTSIZE + (1 << HRGBSHIFT)];
	float Bhist[HISTSIZE + (1 << HRGBSHIFT)];
	float Hhist[HISTSIZE + (1 << HRGBSHIFT)];
	float Shist[HISTSIZE + (1 << HRGBSHIFT)];
	float Vhist[HISTSIZE + (1 << HRGBSHIFT)];

	float Hhr[HISTSIZE + 1];
	float Hhg[HISTSIZE + 1];
	float Hhb[HISTSIZE + 1];

	float Shr[HISTSIZE + 1];
	float Shg[HISTSIZE + 1];
	float Shb[HISTSIZE + 1];

	float Vhr[HISTSIZE + 1];
	float Vhg[HISTSIZE + 1];
	float Vhb[HISTSIZE + 1];
};

class BluebananaUnit : public LoadClient
{
public:
	BluebananaUnit(BluebananaEngine *server, BluebananaMain *plugin);

	void process_package(LoadPackage *package);

	BluebananaEngine *server;
	BluebananaMain *plugin;
};

class BluebananaEngine : public LoadServer
{
public:
	BluebananaEngine(BluebananaMain *plugin,
		int total_clients,
		int total_packages);
	~BluebananaEngine();

	void process_packages(VFrame *data);
	LoadClient* new_client();
	LoadPackage* new_package();

	BluebananaMain *plugin;
	int total_size;
	VFrame *data;

	void set_task(int n, const char *task);
	int next_task();
	void wait_task();

	friend class BluebananaUnit;

protected:
	float *selection_workA;
	float *selection_workB;

private:
	pthread_mutex_t copylock;
	pthread_mutex_t histlock;
	pthread_mutex_t tasklock;
	pthread_cond_t taskcond;
	int task_init_state;
	int task_init_serial;
	int task_n;
	int task_finish_count;
};

extern float sel_lookup(float val, float *lookup);
extern float adj_lookup(float val, float *lookup, float toe_slope);

#endif
