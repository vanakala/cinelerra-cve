
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

#ifndef DEINTERLACE_H
#define DEINTERLACE_H

// the simplest plugin possible

class DeInterlaceMain;

#include "bchash.inc"
#include "deinterwindow.h"
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
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	int mode;
	int adaptive;
	int threshold;
	volatile int dominance; /* top or bottom field */
};

class DeInterlaceMain : public PluginVClient
{
public:
	DeInterlaceMain(PluginServer *server);
	~DeInterlaceMain();


	PLUGIN_CLASS_MEMBERS(DeInterlaceConfig, DeInterlaceThread)
	

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int hide_gui();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	void render_gui(void *data);

	void deinterlace_avg_top(VFrame *input, VFrame *output, int dominance);
	void deinterlace_top(VFrame *input, VFrame *output, int dominance);
	void deinterlace_avg(VFrame *input, VFrame *output);
	void deinterlace_swap(VFrame *input, VFrame *output, int dominance);
	void deinterlace_temporalswap(VFrame *input, VFrame *prevframe, VFrame *output, int dominance);
	void deinterlace_bobweave(VFrame *input, VFrame *prevframe, VFrame *output, int dominance);

	int changed_rows;
	VFrame *temp;
	VFrame *temp_prevframe;
};


#endif
