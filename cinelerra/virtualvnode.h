
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

#include "cropengine.inc"
#include "datatype.h"
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

// Called by VirtualVConsole::process_buffer to process exit nodes.
	void render();

// Read data from what comes before this node.
	VFrame *read_data();
// Get current output_temp
	VFrame *get_output_temp();

private:
	void render_as_module();
	void render_as_plugin();

	VFrame *render_projector();

	void render_fade(Autos *autos);

	void render_mask();

	VRender *vrender;
	FadeEngine *fader;
	MaskEngine *masker;
	CropEngine *cropper;
};


#endif
