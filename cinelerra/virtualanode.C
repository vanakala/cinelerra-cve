
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

#include "aattachmentpoint.h"
#include "aframe.h"
#include "amodule.h"
#include "arender.h"
#include "atrack.h"
#include "automation.h"
#include "bcsignals.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "clip.h"
#include "floatautos.h"
#include "levelhist.h"
#include "mwindow.h"
#include "module.h"
#include "panauto.h"
#include "panautos.h"
#include "plugin.h"
#include "renderengine.h"
#include "track.h"
#include "transition.h"
#include "transportcommand.h"
#include "virtualaconsole.h"
#include "virtualanode.h"


#include <string.h>

VirtualANode::VirtualANode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_module)
 : VirtualNode(renderengine, 
		vconsole,
		real_module, 
		real_plugin,
		track, 
		parent_module)
{
}

VirtualANode::~VirtualANode()
{
}

VirtualNode* VirtualANode::create_module(Plugin *real_plugin, 
	Module *real_module,
	Track *track)
{
	return new VirtualANode(renderengine, 
		vconsole, 
		real_module,
		0,
		track,
		this);
}

VirtualNode* VirtualANode::create_plugin(Plugin *real_plugin)
{
	return new VirtualANode(renderengine, 
		vconsole, 
		0,
		real_plugin,
		track,
		this);
}



int VirtualANode::read_data(AFrame *aframe)
{
	VirtualNode *previous_plugin = 0;

// Current edit in parent track
	Edit *parent_edit = 0;
	if(parent_node && parent_node->track && renderengine)
	{
		parent_edit = parent_node->track->edits->editof(aframe->pts,
			0);
	}

// This is a plugin on parent module with a preceeding effect.
// Get data from preceeding effect on parent module.
	if(parent_node && 
			(previous_plugin = parent_node->get_previous_plugin(this)))
	{
		((VirtualANode*)previous_plugin)->render(aframe);
	}
	else
// The current node is the first plugin on parent module.
// The parent module has an edit to read from or the current node
// has no source to read from.
// Read data from parent module
	if(parent_node && (parent_edit || !real_module))
	{
		((VirtualANode*)parent_node)->read_data(aframe);
	}
	else
	if(real_module)
// This is the first node in the tree
		((AModule*)real_module)->render(aframe);
	return 0;
}

void VirtualANode::render(AFrame *aframe)
{
	ARender *arender = ((VirtualAConsole*)vconsole)->arender;
	if(real_module)
		render_as_module(arender->audio_out, 
			aframe);
	else
	if(real_plugin)
		render_as_plugin(aframe);
}

void VirtualANode::render_as_plugin(AFrame *output_temp)
{
	if(!attachment ||
		!real_plugin ||
		!real_plugin->on) return;

// If we're the first plugin in the parent module, data needs to be read from 
// what comes before the parent module.  Otherwise, data needs to come from the
// previous plugin.
	((AAttachmentPoint*)attachment)->render(
		output_temp, 
		plugin_buffer_number);
}

void VirtualANode::render_as_module(AFrame **audio_out,
	AFrame *output_temp)
{
	int in_output = 0;

// Process last subnode.  This calls read_data, propogates up the chain 
// of subnodes, and finishes the chain.
	if(subnodes.total)
	{
		VirtualANode *node = (VirtualANode*)subnodes.values[subnodes.total - 1];
		node->render(output_temp);
	}
	else
// Read data from previous entity
	{
		read_data(output_temp);
	}

	render_fade(output_temp,
			track->automation->autos[AUTOMATION_FADE]);

	if(real_module && renderengine->command->realtime)
		((AModule*)real_module)->module_levels->fill(&output_temp);

	for(int j = 0; j < MAX_CHANNELS; j++)
	{
		if(audio_out[j]){
			audio_out[j]->pts = output_temp->pts;
			audio_out[j]->samplerate = output_temp->samplerate;
			audio_out[j]->duration = 0;
			audio_out[j]->length = 0;
		}
	}

// process pans and copy the output to the output channels
// Keep rendering unmuted fragments until finished.
	int mute_position = 0;
	ptstime end_pts = output_temp->pts + output_temp->duration;

	for(ptstime pts = output_temp->pts; pts < end_pts;)
	{
		int mute_constant;
		ptstime mute_fragment_project = end_pts - pts;
		int mute_fragment;

// How mutch time until the next mute?
		get_mute_fragment(pts,
				mute_constant, 
				mute_fragment_project,
				(Autos*)track->automation->autos[AUTOMATION_MUTE]);

		mute_fragment = round(mute_fragment_project * output_temp->samplerate);
// Fragment is playable
		for(int j = 0; 
			j < MAX_CHANNELS; 
			j++)
		{
			if(audio_out[j])
			{
				if(!mute_constant)
				{
					render_pan(output_temp,
						audio_out[j],
						mute_position,
						mute_fragment_project,
						(Autos*)track->automation->autos[AUTOMATION_PAN]);
				}
				audio_out[j]->duration += mute_fragment_project;
				audio_out[j]->length += mute_fragment;
			}
		}

		mute_fragment = round(mute_fragment_project * output_temp->samplerate);
		pts += mute_fragment_project;
		mute_position += mute_fragment;
	}
}

void VirtualANode::render_fade(AFrame *aframe, Autos *autos)
{
	double value, fade_value;
	FloatAuto *previous = 0;
	FloatAuto *next = 0;

	ptstime input_postime = aframe->pts;
	ptstime len_pts = aframe->duration;
	int len = aframe->length;

	if(((FloatAutos*)autos)->automation_is_constant(input_postime,
		len_pts,
		fade_value))
	{
		if(EQUIV(fade_value, 0))
			return;
		if(fade_value <= INFINITYGAIN)
			value = 0;
		else
			value = DB::fromdb(fade_value);
		for(int i = 0; i < len; i++)
		{
			aframe->buffer[i] *= value;
		}
	}
	else
	{
		for(int i = 0; i < len; i++)
		{
			fade_value = ((FloatAutos*)autos)->get_value(input_postime + (ptstime)i / aframe->samplerate,
				previous,
				next);
			if(fade_value <= INFINITYGAIN)
				value = 0;
			else
				value = DB::fromdb(fade_value);

			aframe->buffer[i] *= value;
		}
	}
}

void VirtualANode::render_pan(AFrame *iframe, // start of input fragment
	AFrame *oframe,            // start of output fragment
	int offset,               // offset in buffer
	ptstime fragment_duration,
	Autos *autos)
{
	double slope = 0.0;
	double intercept = 1.0;
	double value = 0.0;
	ptstime input_postime = iframe->pts;
	double *input, *output;
	ptstime slope_step = track->one_unit;
	int fragment_len = round(fragment_duration * iframe->samplerate);

	if(fragment_len + offset > iframe->length)
		fragment_len = oframe->length - offset;
	input = iframe->buffer + offset;
	output = oframe->buffer + offset;

	for(int i = 0; i < fragment_len; )
	{
		ptstime slope_len = (ptstime)(fragment_len - i + 1) / iframe->samplerate;
		ptstime slope_max = slope_len;
// Get slope intercept formula for next fragment
		get_pan_automation(slope, 
				intercept,
				input_postime,
				slope_len,
				autos,
				oframe->channel);

		slope_len = MIN(slope_len, slope_max);
		int slope_num = slope_len * iframe->samplerate;

		if(!EQUIV(slope, 0))
		{
			for(double j = 0; j < slope_len && i < fragment_len; j+= slope_step, i++)
			{
				value = slope * j + intercept;
				output[i] += input[i] * value;
			}
		}
		else
		{
			for(int j = 0; j < slope_num && i < fragment_len; j++, i++)
			{
				output[i] += input[i] * intercept;
			}
		}

		input_postime += slope_len;
	}
}


void VirtualANode::get_pan_automation(double &slope,
	double &intercept,
	ptstime input_postime,
	ptstime &slope_len,
	Autos *autos,
	int channel)
{
	intercept = 0;
	slope = 0;

	if(!autos->first)
	{
		intercept = ((PanAutos *)autos)->default_values[channel];
		return;
	}

	PanAuto *prev_keyframe = 0;
	PanAuto *next_keyframe = 0;
	prev_keyframe = (PanAuto*)autos->get_prev_auto(input_postime,
		(Auto* &)prev_keyframe);
	next_keyframe = (PanAuto*)autos->get_next_auto(input_postime,
		(Auto* &)next_keyframe);

	if(next_keyframe->pos_time > prev_keyframe->pos_time)
	{
		slope = ((double)next_keyframe->values[channel] - prev_keyframe->values[channel]) / 
			(next_keyframe->pos_time - prev_keyframe->pos_time);
		intercept = ((double)input_postime - prev_keyframe->pos_time) * slope + 
			prev_keyframe->values[channel];

		if(next_keyframe->pos_time < input_postime + slope_len)
			slope_len = next_keyframe->pos_time - input_postime;
	}
	else
// One automation point within range
	{
		slope = 0;
		intercept = prev_keyframe->values[channel];
	}
}
