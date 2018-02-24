
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
#include "edits.h"
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
	VRender *vrender = ((VirtualVConsole*)vconsole)->vrender;
	fader = new FadeEngine(renderengine->preferences->processors);
	masker = new MaskEngine(renderengine->preferences->processors);
}

VirtualVNode::~VirtualVNode()
{
	delete fader;
	delete masker;
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

VFrame *VirtualVNode::read_data(VFrame *output_temp)
{
	VFrame *video_out = output_temp;
	VirtualNode *previous_plugin = 0;
// If there is a parent module but the parent module has no data source,
// use our own data source.
// Current edit in parent track
	VEdit *parent_edit = 0;
	if(parent_node && parent_node->track && renderengine)
	{
		parent_edit = (VEdit*)parent_node->track->edits->editof(
			output_temp->get_pts(), 0);
	}

// This is a plugin on parent module with a preceeding effect.
// Get data from preceeding effect on parent module.
	if(parent_node && (previous_plugin = parent_node->get_previous_plugin(this)))
	{
		video_out = ((VirtualVNode*)previous_plugin)->render(video_out,
			output_temp);
	}
	else
// The current node is the first plugin on parent module.
// The parent module has an edit to read from or the current node
// has no source to read from.
// Read data from parent module
	if(parent_node && (parent_edit || !real_module))
	{
		video_out = ((VirtualVNode*)parent_node)->read_data(output_temp);
	}
	else
	if(real_module)
	{
// This is the first node in the tree
		video_out = ((VModule*)real_module)->render(output_temp, 0);
	}
	return video_out;
}

VFrame *VirtualVNode::render(VFrame *video_out, VFrame *output_temp)
{
	VRender *vrender = ((VirtualVConsole*)vconsole)->vrender;
	if(real_module)
	{
		render_as_module(video_out,
			output_temp);
	}
	else
	if(real_plugin)
	{
		render_as_plugin(output_temp);
	}
	return video_out;
}

void VirtualVNode::render_as_plugin(VFrame *output_temp)
{
	if(!attachment ||
		!real_plugin ||
		!real_plugin->on) return;

	((VAttachmentPoint*)attachment)->render(
		output_temp,
		plugin_buffer_number, 0);
}

VFrame *VirtualVNode::render_as_module(VFrame *video_out,
	VFrame *output_temp)
{
// Process last subnode.  This propogates up the chain of subnodes and finishes
// the chain.
	if(subnodes.total)
	{
		VirtualVNode *node = (VirtualVNode*)subnodes.values[subnodes.total - 1];
		video_out = node->render(video_out, output_temp);
	}
	else
// Read data from previous entity
	{
		output_temp = read_data(output_temp);
	}

	render_fade(output_temp,
			track->automation->autos[AUTOMATION_FADE]);

	render_mask(output_temp);

// overlay on the final output
// Get mute status
	int mute_constant;
	ptstime mute_fragment = 1;

// Is frame muted?
	get_mute_fragment(output_temp->get_pts(),
		mute_constant,
		mute_fragment,
		(Autos*)((VTrack*)track)->automation->autos[AUTOMATION_MUTE]);

	if(!mute_constant)
	{
// Frame is playable
		render_projector(output_temp, video_out);
	}
	video_out->copy_pts(output_temp);
	return video_out;
}

void VirtualVNode::render_fade(VFrame *output,
			Autos *autos)
{
	double intercept;
	FloatAuto *previous = 0;
	FloatAuto *next = 0;

	intercept = ((FloatAutos*)autos)->get_value(output->get_pts(),
		previous,
		next);

	CLAMP(intercept, 0, 100);

// Can't use overlay here because overlayer blends the frame with itself.
// The fade engine can compensate for lack of alpha channels by multiplying the 
// color components by alpha.
	if(!EQUIV(intercept / 100, 1))
	{
		fader->do_fade(output, output, intercept / 100);
	}
}

void VirtualVNode::render_mask(VFrame *output_temp)
{
	MaskAutos *keyframe_set = 
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK];

	Auto *current = 0;

	MaskAuto *keyframe = (MaskAuto*)keyframe_set->get_prev_auto(output_temp->get_pts(),
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
		output_temp->clear_frame();
		return;
	}

	double edl_rate = renderengine->edl->session->frame_rate;
	masker->do_mask(output_temp, keyframe_set, 0);
}

void VirtualVNode::render_projector(VFrame *input, VFrame *output)
{
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	VRender *vrender = ((VirtualVConsole*)vconsole)->vrender;

	if(output)
	{
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

		if(out_x2 > out_x1 && 
			out_y2 > out_y1 && 
			in_x2 > in_x1 && 
			in_y2 > in_y1)
		{
			IntAuto *mode_keyframe = 0;
			mode_keyframe = 
				(IntAuto*)track->automation->autos[AUTOMATION_MODE]->get_prev_auto(
					input->get_pts(), 
					(Auto* &)mode_keyframe);

			int mode;
			if(mode_keyframe)
				mode = mode_keyframe->value;
			else
				mode = TRANSFER_NORMAL;

			vrender->overlayer->overlay(output,
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
