// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DEINTERLACE_H
#define DEINTERLACE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_STATUS_GUI
#define PLUGIN_NOT_KEYFRAMEABLE

#define PLUGIN_TITLE N_("Deinterlace")
#define PLUGIN_CLASS DeInterlaceMain
#define PLUGIN_CONFIG_CLASS DeInterlaceConfig
#define PLUGIN_THREAD_CLASS DeInterlaceThread
#define PLUGIN_GUI_CLASS DeInterlaceWindow

#include "pluginmacros.h"

#include "bchash.inc"
#include "deinterwindow.h"
#include "language.h"
#include "pluginvclient.h"
#include "vframe.inc"

#define THRESHOLD_SCALAR 1000

enum
{
	DEINTERLACE_NONE,
	DEINTERLACE_KEEP,
	DEINTERLACE_AVG_1F,
	DEINTERLACE_AVG,
	DEINTERLACE_BOBWEAVE,
	DEINTERLACE_SWAP,
	DEINTERLACE_TEMPORALSWAP,
};

class DeInterlaceConfig
{
public:
	DeInterlaceConfig();

	int equivalent(DeInterlaceConfig &that);
	void copy_from(DeInterlaceConfig &that);
	void interpolate(DeInterlaceConfig &prev, 
		DeInterlaceConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int mode;
	int adaptive;
	int threshold;
	volatile int dominance; /* top or bottom field */
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class DeInterlaceMain : public PluginVClient
{
public:
	DeInterlaceMain(PluginServer *server);
	~DeInterlaceMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	VFrame *process_tmpframe(VFrame *frame);

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void render_gui(void *data);

	VFrame *deinterlace_avg_top(VFrame *input, VFrame *output, int dominance);
	void deinterlace_top(VFrame *input, VFrame *output, int dominance);
	void deinterlace_avg(VFrame *input, VFrame *output);
	void deinterlace_swap(VFrame *input, VFrame *output, int dominance);
	void deinterlace_temporalswap(VFrame *input, VFrame *prevframe, VFrame *output, int dominance);
	void deinterlace_bobweave(VFrame *input, VFrame *prevframe, VFrame *output, int dominance);

	int changed_rows;
};

#endif
