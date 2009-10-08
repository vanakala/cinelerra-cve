
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

#ifndef VIRTUALVNODE_H
#define VIRTUALVNODE_H

#include "fadeengine.inc"
#include "maskengine.inc"
#include "plugin.inc"
#include "renderengine.inc"
#include "vframe.inc"
#include "virtualnode.h"
#include "vrender.inc"

class VirtualVNode : public VirtualNode
{
public:
// construct as a module or a plugin
	VirtualVNode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_module);

	~VirtualVNode();

// expansions
	VirtualNode* create_module(Plugin *real_plugin, 
							Module *real_module, 
							Track *track);
	VirtualNode* create_plugin(Plugin *real_plugin);
	void arm_attachmentpoint();

// Called by VirtualVConsole::process_buffer to process exit nodes.
// start_position - end of frame if reverse.  start of frame if forward.
// frame_rate - rate start_position is relative to
// use_opengl - if opengl is available for this step
	int render(VFrame *output_temp, 
		int64_t start_position,
		double frame_rate,
		int use_opengl);

// Read data from what comes before this node.
	int read_data(VFrame *output_temp,
		int64_t start_position,
		double frame_rate,
		int use_opengl);

private:
	int render_as_module(VFrame *video_out, 
		VFrame *output_temp,
		int64_t start_position,
		double frame_rate,
		int use_opengl);
	void render_as_plugin(VFrame *output_temp, 
		int64_t start_position,
		double frame_rate,
		int use_opengl);

	int render_projector(VFrame *input,
			VFrame *output,
			int64_t start_position,
			double frame_rate);  // Start of input fragment in project if forward.  End of input fragment if reverse.

	int render_fade(VFrame *output,        // start of output fragment
			int64_t start_position,  // start of input fragment in project if forward / end of input fragment if reverse
			double frame_rate, 
			Autos *autos,
			int direction);

	void render_mask(VFrame *output_temp,
		int64_t start_position_project,
		double frame_rate,
		int use_opengl);

	FadeEngine *fader;
	MaskEngine *masker;
};


#endif
