
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

#include "automation.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "clip.h"
#include "cropengine.h"
#include "edl.h"
#include "edlsession.h"
#include "fadeengine.h"
#include "floatauto.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
#include "maskauto.h"
#include "maskautos.h"
#include "maskengine.h"
#include "module.h"
#include "overlayframe.h"
#include "plugin.h"
#include "preferences.h"
#include "renderengine.h"
#include "tmpframecache.h"
#include "vattachmentpoint.h"
#include "vframe.h"
#include "virtualvconsole.h"
#include "virtualvnode.h"
#include "vmodule.h"
#include "vrender.h"
#include "vtrack.h"

#include <string.h>


VirtualVNode::VirtualVNode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_node)
 : VirtualNode(renderengine, 
		vconsole, 
		real_module, 
		real_plugin,
		track, 
		parent_node)
{
	vrender = ((VirtualVConsole*)vconsole)->vrender;
	fader = 0;
	masker = 0;
	cropper = 0;
}

VirtualVNode::~VirtualVNode()
{
	delete fader;
	delete masker;
	delete cropper;
}

VirtualNode* VirtualVNode::create_module(Plugin *real_plugin, 
					Module *real_module, 
					Track *track)
{
	return new VirtualVNode(renderengine, 
		vconsole, 
		real_module,
		0,
		track,
		this);
}

VirtualNode* VirtualVNode::create_plugin(Plugin *real_plugin)
{
	return new VirtualVNode(renderengine, 
		vconsole, 
		0,
		real_plugin,
		track,
		this);
}

VFrame *VirtualVNode::get_output_temp()
{
	return ((VirtualVConsole*)vconsole)->output_temp;
}

VFrame *VirtualVNode::read_data()
{
	VFrame *output_temp = ((VirtualVConsole*)vconsole)->output_temp;
	VirtualNode *previous_plugin = 0;

// If there is a parent module but the parent module has no data source,
// use our own data source.
// Current edit in parent track
	Edit *parent_edit = 0;
	if(parent_node && parent_node->track && renderengine)
	{
		parent_edit = parent_node->track->editof(
			output_temp->get_pts());
	}

// This is a plugin on parent module with a preceeding effect.
// Get data from preceeding effect on parent module.
	if(parent_node && (previous_plugin = parent_node->get_previous_plugin(this)))
	{
		((VirtualVNode*)previous_plugin)->render();
	}
	else
// The current node is the first plugin on parent module.
// The parent module has an edit to read from or the current node
// has no source to read from.
// Read data from parent module
	if(parent_node && (parent_edit || !real_module))
	{
		((VirtualVNode*)parent_node)->read_data();
	}
	else
	if(real_module)
	{
// This is the first node in the tree
		((VModule*)real_module)->render(output_temp);
	}
	return output_temp;
}

void VirtualVNode::render()
{
	if(real_module)
	{
		render_as_module();
	}
	else
	if(real_plugin)
	{
		render_as_plugin();
	}
}

void VirtualVNode::render_as_plugin()
{
	if(!attachment ||
		!real_plugin ||
		!real_plugin->on) return;

	((VAttachmentPoint*)attachment)->render(
		&((VirtualVConsole*)vconsole)->output_temp,
		plugin_buffer_number);
}

void VirtualVNode::render_as_module()
{
// Process last subnode.  This propogates up the chain of subnodes and finishes
// the chain.
	if(subnodes.total)
	{
		VirtualVNode *node = (VirtualVNode*)subnodes.values[subnodes.total - 1];
		node->render();
	}
	else
// Read data from previous entity
	{
		read_data();
	}

	render_fade(track->automation->autos[AUTOMATION_FADE]);

	render_mask();

	if(!cropper)
		cropper = new CropEngine();

	((VirtualVConsole*)vconsole)->output_temp = cropper->do_crop(
		track->automation->autos[AUTOMATION_CROP],
		((VirtualVConsole*)vconsole)->output_temp, 0);

// overlay on the final output
// Get mute status
	int mute_constant;
	ptstime mute_fragment = 1;

// Is frame muted?
	get_mute_fragment(((VirtualVConsole*)vconsole)->output_temp->get_pts(),
		mute_constant,
		mute_fragment,
		(Autos*)((VTrack*)track)->automation->autos[AUTOMATION_MUTE]);

	vrender->video_out->set_duration(edlsession->frame_duration());

	if(!mute_constant)
	{
// Frame is playable
		render_projector();
	}
	return;
}

void VirtualVNode::render_fade(Autos *autos)
{
	double intercept;
	FloatAuto *previous = 0;
	FloatAuto *next = 0;
	VFrame *output = ((VirtualVConsole*)vconsole)->output_temp;

	intercept = ((FloatAutos*)autos)->get_value(output->get_pts(),
		previous,
		next);

	CLAMP(intercept, 0, 100);

// Can't use overlay here because overlayer blends the frame with itself.
// The fade engine can compensate for lack of alpha channels by multiplying the 
// color components by alpha.
	if(!EQUIV(intercept / 100, 1))
	{
		if(!fader)
			fader = new FadeEngine(preferences_global->processors);
		fader->do_fade(output, output, intercept / 100);
		// Colormodels with alpha - only alpha is modified
		if(ColorModels::has_alpha(output->get_color_model()))
			output->set_transparent();
	}
}

void VirtualVNode::render_mask()
{
	VFrame *output = ((VirtualVConsole*)vconsole)->output_temp;
	MaskAutos *keyframe_set = 
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK];

	Auto *current = 0;

	MaskAuto *keyframe = (MaskAuto*)keyframe_set->get_prev_auto(output->get_pts(),
		current);

	if(!keyframe)
		return;

	int total_points = 0;
	for(int i = 0; i < keyframe->masks.total; i++)
	{
		SubMask *mask = keyframe->get_submask(i);
		int submask_points = mask->points.total;
		if(submask_points > 1) total_points += submask_points;
	}

// Ignore certain masks
	if(total_points <= 2 || 
		(keyframe->value == 0 && keyframe_set->get_mode() == MASK_SUBTRACT_ALPHA))
	{
		return;
	}

// Fake certain masks
	if(keyframe->value == 0 && keyframe_set->get_mode() == MASK_MULTIPLY_ALPHA)
	{
		output->clear_frame();
		return;
	}

	if(!masker)
		masker = new MaskEngine(preferences_global->processors);

	masker->do_mask(output, keyframe_set, 0);
	// Colormodels with alpha - only alpha is modified
	if(ColorModels::has_alpha(output->get_color_model()))
		output->set_transparent();
}

VFrame *VirtualVNode::render_projector()
{
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	VFrame *tmpframe;
	VFrame *input = ((VirtualVConsole*)vconsole)->output_temp;
	int mode;

	((VTrack*)track)->calculate_output_transfer(input->get_pts(),
		&in_x1,
		&in_y1,
		&in_x2,
		&in_y2,
		&out_x1,
		&out_y1,
		&out_x2,
		&out_y2);

	in_x2 += in_x1;
	in_y2 += in_y1;
	out_x2 += out_x1;
	out_y2 += out_y1;

	if(out_x2 > out_x1 && out_y2 > out_y1 &&
		in_x2 > in_x1 && in_y2 > in_y1)
	{
		IntAuto *mode_keyframe = 0;
		mode_keyframe =
			(IntAuto*)track->automation->autos[AUTOMATION_MODE]->get_prev_auto(
				input->get_pts(),
				(Auto* &)mode_keyframe);

		if(mode_keyframe)
			mode = mode_keyframe->value;
		else
			mode = TRANSFER_NORMAL;

		if(mode == TRANSFER_NORMAL && !(input->get_status() & VFRAME_TRANSPARENT) &&
			vconsole->current_exit_node == vconsole->total_exit_nodes - 1 &&
			in_x1 == out_x1 && in_x2 == out_x2 &&
			in_y1 == out_y1 && in_y2 == out_y2)
		{
			tmpframe = vrender->video_out;
			vrender->video_out = input;
			((VirtualVConsole*)vconsole)->output_temp = tmpframe;
		}
		else
		{
			vrender->overlayer->overlay(vrender->video_out,
				input,
				in_x1,
				in_y1,
				in_x2,
				in_y2,
				out_x1,
				out_y1,
				out_x2,
				out_y2,
				1,
				mode,
				BC_Resources::interpolation_method);
		}
	}
}
