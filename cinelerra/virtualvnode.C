#include "asset.h"
#include "automation.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "fadeengine.h"
#include "floatauto.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
#include "maskengine.h"
#include "mwindow.h"
#include "module.h"
#include "overlayframe.h"
#include "playabletracks.h"
#include "plugin.h"
#include "preferences.h"
#include "renderengine.h"
#include "transition.h"
#include "transportque.h"
#include "vattachmentpoint.h"
#include "vedit.h"
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
}

VirtualVNode::~VirtualVNode()
{
	delete fader;
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

int VirtualVNode::read_data(VFrame *output_temp,
	int64_t start_position,
	double frame_rate)
{
	VirtualNode *previous_plugin = 0;

	if(!output_temp) 
		printf("VirtualVNode::read_data output_temp=%p\n", output_temp);

	if(vconsole->debug_tree) 
		printf("  VirtualVNode::read_data position=%lld rate=%f title=%s\n", 
			start_position,
			frame_rate,
			track->title);

// This is a plugin on parent module with a preceeding effect.
// Get data from preceeding effect on parent module.
	if(parent_node && (previous_plugin = parent_node->get_previous_plugin(this)))
	{
		return ((VirtualVNode*)previous_plugin)->render(output_temp,
			start_position,
			frame_rate);
	}
	else
// First plugin on parent module.
// Read data from parent module
	if(parent_node)
	{
		return ((VirtualVNode*)parent_node)->read_data(output_temp,
			start_position,
			frame_rate);
	}
	else
	{
// This is the first node in the tree
		return ((VModule*)real_module)->render(output_temp,
			start_position,
			renderengine->command->get_direction(),
			frame_rate,
			0,
			vconsole->debug_tree);
	}

	return 0;
}


int VirtualVNode::render(VFrame *output_temp, 
	int64_t start_position,
	double frame_rate)
{
	VRender *vrender = ((VirtualVConsole*)vconsole)->vrender;
	if(real_module)
	{
		render_as_module(vrender->video_out, 
			output_temp,
			start_position,
			frame_rate);
	}
	else
	if(real_plugin)
	{
		render_as_plugin(output_temp,
			start_position,
			frame_rate);
	}
	return 0;
}

void VirtualVNode::render_as_plugin(VFrame *output_temp, 
	int64_t start_position,
	double frame_rate)
{
	if(!attachment ||
		!real_plugin ||
		!real_plugin->on) return;


	if(vconsole->debug_tree) 
		printf("  VirtualVNode::render_as_plugin title=%s\n", track->title);

	((VAttachmentPoint*)attachment)->render(
		output_temp,
		plugin_buffer_number,
		start_position,
		frame_rate,
		vconsole->debug_tree);
}


int VirtualVNode::render_as_module(VFrame **video_out, 
	VFrame *output_temp,
	int64_t start_position,
	double frame_rate)
{

	int direction = renderengine->command->get_direction();
	double edl_rate = renderengine->edl->session->frame_rate;

	if(vconsole->debug_tree) 
		printf("  VirtualVNode::render_as_module title=%s\n", 
			track->title);

// Process last subnode.  This propogates up the chain of subnodes and finishes
// the chain.
	if(subnodes.total)
	{
		VirtualVNode *node = (VirtualVNode*)subnodes.values[subnodes.total - 1];
		node->render(output_temp,
			start_position,
			frame_rate);
	}
	else
// Read data from previous entity
	{
		read_data(output_temp,
			start_position,
			frame_rate);
	}

	render_fade(output_temp,
				start_position,
				frame_rate,
				track->automation->autos[AUTOMATION_FADE],
				direction);

// Apply mask to output
	((VModule *)real_module)->masker->do_mask(output_temp, 
		start_position,
		frame_rate,
		edl_rate,
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK], 
		direction,
		0);      // we are not before plugins


// overlay on the final output
// Get mute status
	int mute_constant;
	int mute_fragment = 1;
	int64_t mute_position = 0;


// Is frame muted?
	get_mute_fragment(start_position,
			mute_constant, 
			mute_fragment, 
			(Autos*)((VTrack*)track)->automation->autos[AUTOMATION_MUTE],
			direction,
			0);

	if(!mute_constant)
	{
// Fragment is playable
		render_projector(output_temp,
			video_out,
			start_position,
			frame_rate);
	}


	Edit *edit = 0;
	if(renderengine->show_tc)
		renderengine->vrender->insert_timecode(edit,
			start_position,
			output_temp);

	return 0;
}

int VirtualVNode::render_fade(VFrame *output,        
// start of input fragment in project if forward / end of input fragment if reverse
// relative to requested frame rate
			int64_t start_position, 
			double frame_rate, 
			Autos *autos,
			int direction)
{
	double slope, intercept;
	int64_t slope_len = 1;
	FloatAuto *previous = 0;
	FloatAuto *next = 0;
	double edl_rate = renderengine->edl->session->frame_rate;
	int64_t start_position_project = (int64_t)(start_position * 
		edl_rate /
		frame_rate);

	if(vconsole->debug_tree) 
		printf("  VirtualVNode::render_fade title=%s\n", track->title);

	intercept = ((FloatAutos*)autos)->get_value(start_position_project, 
		direction,
		previous,
		next);


//	CLAMP(intercept, 0, 100);


// Can't use overlay here because overlayer blends the frame with itself.
// The fade engine can compensate for lack of alpha channels by reducing the 
// color components.
	if(!EQUIV(intercept / 100, 1))
	{
		fader->do_fade(output, output, intercept / 100);
	}

	return 0;
}

// Start of input fragment in project if forward.  End of input fragment if reverse.
int VirtualVNode::render_projector(VFrame *input,
			VFrame **output,
			int64_t start_position,
			double frame_rate)
{
	float in_x1, in_y1, in_x2, in_y2;
	float out_x1, out_y1, out_x2, out_y2;
	double edl_rate = renderengine->edl->session->frame_rate;
	int64_t start_position_project = (int64_t)(start_position * 
		edl_rate /
		frame_rate);
	VRender *vrender = ((VirtualVConsole*)vconsole)->vrender;
	if(vconsole->debug_tree) 
		printf("  VirtualVNode::render_projector title=%s\n", track->title);

	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		if(output[i])
		{
			((VTrack*)track)->calculate_output_transfer(i,
				start_position_project,
				renderengine->command->get_direction(),
				in_x1, 
				in_y1, 
				in_x2, 
				in_y2,
				out_x1, 
				out_y1, 
				out_x2, 
				out_y2);

			in_x2 += in_x1;
			in_y2 += in_y1;
			out_x2 += out_x1;
			out_y2 += out_y1;

//for(int j = 0; j < input->get_w() * 3 * 5; j++)
//	input->get_rows()[0][j] = 255;
// 
			if(out_x2 > out_x1 && 
				out_y2 > out_y1 && 
				in_x2 > in_x1 && 
				in_y2 > in_y1)
			{
 				int direction = renderengine->command->get_direction();
				IntAuto *mode_keyframe = 0;
				mode_keyframe = 
					(IntAuto*)track->automation->autos[AUTOMATION_MODE]->get_prev_auto(
						start_position_project, 
						direction,
						(Auto* &)mode_keyframe);

				int mode = mode_keyframe->value;

// Fade is performed in render_fade so as to allow this module
// to be chained in another module, thus only 4 component colormodels
// can do dissolves, although a blend equation is still required for 3 component
// colormodels since fractional translation requires blending.

// If this is the only playable video track and the mode_keyframe is "normal"
// the mode keyframe may be overridden with "replace".  Replace is faster.
				if(mode == TRANSFER_NORMAL &&
					vconsole->total_entry_nodes == 1)
					mode = TRANSFER_REPLACE;


				vrender->overlayer->overlay(output[i], 
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
					renderengine->edl->session->interpolation_type);
			}
// for(int j = 0; j < output[i]->get_w() * 3 * 5; j++)
//  	output[i]->get_rows()[0][j] = 255;
		}
	}
	return 0;
}

