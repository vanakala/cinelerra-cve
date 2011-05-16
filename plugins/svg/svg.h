
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

#ifndef SVG_H
#define SVG_H

// the simplest plugin possible

class SvgMain;
class SvgThread;

#include "bchash.h"
#include "mutex.h"
#include "svgwin.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "thread.h"

class SvgConfig
{
public:
	SvgConfig();
	int equivalent(SvgConfig &that);
	void copy_from(SvgConfig &that);
	void interpolate(SvgConfig &prev, 
		SvgConfig &next, 
		posnum prev_frame, 
		posnum next_frame, 
		posnum current_frame);

	float in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h;
	char svg_file[BCTEXTLEN];
	int64_t last_load;
};


class SvgMain : public PluginVClient
{
public:
	SvgMain(PluginServer *server);
	~SvgMain();

	PLUGIN_CLASS_MEMBERS(SvgConfig, SvgThread);

// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	int is_synthesis();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	OverlayFrame *overlayer;   // To translate images
	VFrame *temp_frame;        // Used if buffers are the same
	int need_reconfigure;
	int force_raw_render;     //force rendering of PNG on first start
};


#endif
