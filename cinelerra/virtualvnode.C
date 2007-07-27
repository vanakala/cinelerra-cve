#include "asset.h"
#include "automation.h"
#include "bcsignals.h"
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
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"
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

int VirtualVNode::read_data(VFrame *output_temp,
	int64_t start_position,
	double frame_rate,
	int use_opengl)
{
	VirtualNode *previous_plugin = 0;
	int result = 0;

	if(!output_temp) 
		printf("VirtualVNode::read_data output_temp=%p\n", output_temp);

	if(vconsole->debug_tree) 
		printf("  VirtualVNode::read_data position=%lld rate=%f title=%s opengl=%d\n", 
			start_position,
			frame_rate,
			track->title, 
			use_opengl);

// If there is a parent module but the parent module has no data source,
// use our own data source.
// Current edit in parent track
	VEdit *parent_edit = 0;
	if(parent_node && parent_node->track && renderengine)
	{
		double edl_rate = renderengine->edl->session->frame_rate;
		int64_t start_position_project = (int64_t)(start_position *
			edl_rate /
			frame_rate + 
			0.5);
		parent_edit = (VEdit*)parent_node->track->edits->editof(start_position_project, 
			renderengine->command->get_direction(),
			0);
	}


// This is a plugin on parent module with a preceeding effect.
// Get data from preceeding effect on parent module.
	if(parent_node && (previous_plugin = parent_node->get_previous_plugin(this)))
	{
		result = ((VirtualVNode*)previous_plugin)->render(output_temp,
			start_position,
			frame_rate,
			use_opengl);
	}
	else
// The current node is the first plugin on parent module.
// The parent module has an edit to read from or the current node
// has no source to read from.
// Read data from parent module
	if(parent_node && (parent_edit || !real_module))
	{
		result = ((VirtualVNode*)parent_node)->read_data(output_temp,
			start_position,
			frame_rate,
			use_opengl);
	}
	else
	if(real_module)
	{
// This is the first node in the tree
		result = ((VModule*)real_module)->render(output_temp,
			start_position,
			renderengine->command->get_direction(),
			frame_rate,
			0,
			vconsole->debug_tree,
			use_opengl);
	}

	return result;
}


int VirtualVNode::render(VFrame *output_temp, 
	int64_t start_position,
	double frame_rate,
	int use_opengl)
{
	VRender *vrender = ((VirtualVConsole*)vconsole)->vrender;
	if(real_module)
	{
		render_as_module(vrender->video_out, 
			output_temp,
			start_position,
			frame_rate,
			use_opengl);
	}
	else
	if(real_plugin)
	{
		render_as_plugin(output_temp,
			start_position,
			frame_rate,
			use_opengl);
	}
	return 0;
}

void VirtualVNode::render_as_plugin(VFrame *output_temp, 
	int64_t start_position,
	double frame_rate,
	int use_opengl)
{
	if(!attachment ||
		!real_plugin ||
		!real_plugin->on) return;


	if(vconsole->debug_tree) 
		printf("  VirtualVNode::render_as_plugin title=%s use_opengl=%d\n", 
			track->title,
			use_opengl);

	((VAttachmentPoint*)attachment)->render(
		output_temp,
		plugin_buffer_number,
		start_position,
		frame_rate,
		vconsole->debug_tree,
		use_opengl);
}


int VirtualVNode::render_as_module(VFrame *video_out, 
	VFrame *output_temp,
	int64_t start_position,
	double frame_rate,
	int use_opengl)
{

	int direction = renderengine->command->get_direction();
	double edl_rate = renderengine->edl->session->frame_rate;
// Get position relative to project, compensated for direction
	int64_t start_position_project = (int64_t)(start_position *
		edl_rate / 
		frame_rate);
	if(direction == PLAY_REVERSE) start_position_project--;

	if(vconsole->debug_tree) 
		printf("  VirtualVNode::render_as_module title=%s use_opengl=%d video_out=%p output_temp=%p\n", 
			track->title,
			use_opengl,
			video_out,
			output_temp);

	output_temp->push_next_effect("VirtualVNode::render_as_module");

// Process last subnode.  This propogates up the chain of subnodes and finishes
// the chain.
	if(subnodes.total)
	{
		VirtualVNode *node = (VirtualVNode*)subnodes.values[subnodes.total - 1];
		node->render(output_temp,
			start_position,
			frame_rate,
			use_opengl);
	}
	else
// Read data from previous entity
	{
		read_data(output_temp,
			start_position,
			frame_rate,
			use_opengl);
	}

	output_temp->pop_next_effect();

	render_fade(output_temp,
				start_position,
				frame_rate,
				track->automation->autos[AUTOMATION_FADE],
				direction);

	render_mask(output_temp, start_position_project, frame_rate, use_opengl);


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
// Frame is playable
		render_projector(output_temp,
			video_out,
			start_position,
			frame_rate);
	}

	output_temp->push_prev_effect("VirtualVNode::render_as_module");
//printf("VirtualVNode::render_as_module\n");
//output_temp->dump_stacks();

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


	CLAMP(intercept, 0, 100);


// Can't use overlay here because overlayer blends the frame with itself.
// The fade engine can compensate for lack of alpha channels by multiplying the 
// color components by alpha.
	if(!EQUIV(intercept / 100, 1))
	{
		if(((VirtualVConsole*)vconsole)->use_opengl)
			((VDeviceX11*)((VirtualVConsole*)vconsole)->get_vdriver())->do_fade(
				output, 
				intercept / 100);
		else
			fader->do_fade(output, output, intercept / 100);
	}

	return 0;
}



void VirtualVNode::render_mask(VFrame *output_temp,
	int64_t start_position_project,
	double frame_rate,
	int use_opengl)
{
	MaskAutos *keyframe_set = 
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK];

	Auto *current = 0;
	MaskAuto *default_auto = (MaskAuto*)keyframe_set->default_auto;
	MaskAuto *keyframe = (MaskAuto*)keyframe_set->get_prev_auto(start_position_project, 
		PLAY_FORWARD,
		current);

	int total_points = 0;
	for(int i = 0; i < keyframe->masks.total; i++)
	{
		SubMask *mask = keyframe->get_submask(i);
		int submask_points = mask->points.total;
		if(submask_points > 1) total_points += submask_points;
	}

//printf("VirtualVNode::render_mask 1 %d %d\n", total_points, keyframe->value);
// Ignore certain masks
	if(total_points <= 2 || 
		(keyframe->value == 0 && default_auto->mode == MASK_SUBTRACT_ALPHA))
	{
		return;
	}

// Fake certain masks
	if(keyframe->value == 0 && default_auto->mode == MASK_MULTIPLY_ALPHA)
	{
		output_temp->clear_frame();
		return;
	}

	if(((VirtualVConsole*)vconsole)->use_opengl)
	{
		((VDeviceX11*)((VirtualVConsole*)vconsole)->get_vdriver())->do_mask(
			output_temp, 
			start_position_project,
			keyframe_set, 
			keyframe,
			default_auto);
	}
	else
	{
// Revert to software
		int direction = renderengine->command->get_direction();
		double edl_rate = renderengine->edl->session->frame_rate;
		masker->do_mask(output_temp, 
			start_position_project,
			frame_rate,
			edl_rate,
			keyframe_set, 
			direction,
			0);
	}
}


// Start of input fragment in project if forward.  End of input fragment if reverse.
int VirtualVNode::render_projector(VFrame *input,
			VFrame *output,
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
		printf("  VirtualVNode::render_projector input=%p output=%p title=%s\n", 
			input, output, track->title);

	if(output)
	{
		((VTrack*)track)->calculate_output_transfer(start_position_project,
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

// If this is the first playable video track and the mode_keyframe is "normal"
// the mode may be overridden with "replace".  Replace is faster.
			if(mode == TRANSFER_NORMAL &&
				vconsole->current_exit_node == vconsole->total_exit_nodes - 1)
				mode = TRANSFER_REPLACE;

			if(((VirtualVConsole*)vconsole)->use_opengl)
			{
				((VDeviceX11*)((VirtualVConsole*)vconsole)->get_vdriver())->overlay(
					output,
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
					renderengine->edl);
			}
			else
			{
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
					renderengine->edl->session->interpolation_type);
			}
		}
	}
	return 0;
}

