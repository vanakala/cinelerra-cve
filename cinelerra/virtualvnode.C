#include "automation.h"
#include "clip.h"
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
#include "plugin.h"
#include "preferences.h"
#include "renderengine.h"
#include "transition.h"
#include "transportque.h"
#include "vattachmentpoint.h"
#include "vframe.h"
#include "virtualvconsole.h"
#include "virtualvnode.h"
#include "vmodule.h"
#include "vtrack.h"

#include <string.h>


VirtualVNode::VirtualVNode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_module, 
		VFrame *buffer_in,
		VFrame *buffer_out,
		int input_is_master,
		int output_is_master,
		int in,
		int out)
 : VirtualNode(renderengine, 
		vconsole, 
		real_module, 
		real_plugin,
		track, 
		parent_module, 
		input_is_master,
		output_is_master,
		in,
		out)
{
	this->buffer_in = buffer_in;
	this->buffer_out = buffer_out;
//printf("VirtualVNode::VirtualVNode 1\n");
	overlayer = new OverlayFrame(renderengine->preferences->processors);
	fader = new FadeEngine(renderengine->preferences->processors);
	masker = new MaskEngine(renderengine->preferences->processors);
}

VirtualVNode::~VirtualVNode()
{
//printf("VirtualVNode::~VirtualVNode 1\n");
	if(!shared_output) delete buffer_out;
	if(!shared_input) delete buffer_in;
	delete overlayer;
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
		this,
		data_in_input ? buffer_in : buffer_out,
		buffer_out,
		data_in_input ? input_is_master : output_is_master,
		output_is_master,
		real_plugin->in,
		real_plugin->out);
}


VirtualNode* VirtualVNode::create_plugin(Plugin *real_plugin)
{
	return new VirtualVNode(renderengine, 
		vconsole, 
		0,
		real_plugin,
		track,
		this,
		data_in_input ? buffer_in : buffer_out,
		buffer_out,
		data_in_input ? input_is_master : output_is_master,
		output_is_master,
		real_plugin->in,
		real_plugin->out);
}


void VirtualVNode::new_output_buffer()
{
printf("VirtualVNode::new_input_buffer 1 %p\n", track);
	buffer_out = new VFrame(0,
			track->track_w,
			track->track_h,
			renderengine->edl->session->color_model,
			-1);
}

void VirtualVNode::new_input_buffer()
{
printf("VirtualVNode::new_input_buffer 1 %p\n", track);
	buffer_in = new VFrame(0,
			track->track_w,
			track->track_h,
			renderengine->edl->session->color_model,
			-1);
}

VFrame* VirtualVNode::get_module_input()
{
	VFrame* result;
	if(data_in_input)
	{
		result = buffer_in;
	}
	else
	{
		result = buffer_out;
	}
	return result;
}

VFrame* VirtualVNode::get_module_output()
{
	VFrame* result;
	result = buffer_out;
	return result;
}

int VirtualVNode::render(VFrame **video_out, int64_t input_position)
{
	if(real_module)
	{
		render_as_module(video_out, input_position);
	}
	else
	if(real_plugin)
	{
		render_as_plugin(input_position);
	}
	return 0;
}

void VirtualVNode::render_as_plugin(int64_t input_position)
{
	if(!attachment ||
		!real_plugin ||
		!real_plugin->on) return;
	input_position += track->nudge;

	((VAttachmentPoint*)attachment)->render(buffer_in,
		buffer_out,
		input_position);
}


int VirtualVNode::render_as_module(VFrame **video_out, int64_t input_position)
{
	this->reverse = reverse;
	VFrame *buffer_in = get_module_input();
	VFrame *buffer_out = get_module_output();
	int direction = renderengine->command->get_direction();

	input_position += track->nudge;


	render_fade(buffer_in, 
				buffer_out,
				input_position,
				track->automation->fade_autos,
				direction);


// video is definitely in output buffer now

// Render mask
	masker->do_mask(buffer_out, 
		track->automation->mask_autos, 
		input_position, 
		renderengine->command->get_direction());









// overlay on the final output
// Get mute status
	int mute_constant;
	int64_t mute_fragment = 1;
	int64_t mute_position = 0;

//printf("VirtualVNode::render_as_module 3\n");
// Is frame muted?
	get_mute_fragment(input_position,
			mute_constant, 
			mute_fragment, 
			(Autos*)((VTrack*)track)->automation->mute_autos,
			direction,
			0);

	if(!mute_constant)
	{
// Fragment is playable
		render_projector(buffer_out,
			video_out,
			input_position);
	}

//printf("VirtualVNode::render_as_module 5\n");
	return 0;
}

int VirtualVNode::render_fade(VFrame *input,         // start of input fragment
			VFrame *output,        // start of output fragment
			int64_t input_position,  // start of input fragment in project if forward / end of input fragment if reverse
			Autos *autos,
			int direction)
{
	double slope, intercept;
	int64_t slope_len = 1;
	FloatAuto *previous = 0;
	FloatAuto *next = 0;

	intercept = ((FloatAutos*)autos)->get_value(input_position, 
		direction,
		previous,
		next);


//printf("VirtualVNode::render_fade %d %f\n", input_position, intercept);
	CLAMP(intercept, 0, 100);


// Can't use overlay here because overlayer blends the frame with itself.
// The fade engine can compensate for lack of alpha channels by reducing the 
// color components.
	if(!EQUIV(intercept / 100, 1))
	{
		fader->do_fade(output, input, intercept / 100);
	}
	else
	{
		if(output->get_rows()[0] != input->get_rows()[0])
		{
			output->copy_from(input);
		}
	}

	return 0;
}

// Start of input fragment in project if forward.  End of input fragment if reverse.
int VirtualVNode::render_projector(VFrame *input,
			VFrame **output,
			int64_t input_position)
{
	float in_x1, in_y1, in_x2, in_y2;
	float out_x1, out_y1, out_x2, out_y2;

	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		if(output[i])
		{
			((VTrack*)track)->calculate_output_transfer(i,
				input_position,
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
					(IntAuto*)track->automation->mode_autos->get_prev_auto(
						input_position, 
						direction,
						(Auto*)mode_keyframe);

				int mode = mode_keyframe->value;

// Fade is performed in render_fade so as to allow this module
// to be chained in another module, thus only 4 component colormodels
// can do dissolves, although a blend equation is still required for 3 component
// colormodels since fractional translation requires blending.

// If this is the only playable video track and the mode_keyframe is "normal"
// the mode keyframe may be overridden with "replace".  Replace is faster.
				if(mode == TRANSFER_NORMAL &&
					vconsole->total_tracks == 1)
					mode = TRANSFER_REPLACE;


				overlayer->overlay(output[i], 
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

int VirtualVNode::transfer_from(VFrame *frame_out, 
	VFrame *frame_in, 
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2, 
	float alpha, 
	int mode)
{
printf("VirtualVNode::transfer_from Depreciated\n", mode, alpha);
// 	overlayer->overlay(frame_out, 
// 		frame_in,
// 		in_x1, 
// 		in_y1, 
// 		in_x2, 
// 		in_y2,
// 		out_x1, 
// 		out_y1, 
// 		out_x2, 
// 		out_y2, 
// 		1,
// 		mode, 
// 		renderengine->edl->session->interpolation_type);
	return 0;
}
