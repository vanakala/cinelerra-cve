#include "asset.h"
#include "cache.h"
#include "clip.h"
#include "commonrender.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "overlayframe.h"
#include "patch.h"
#include "preferences.h"
#include "renderengine.h"
#include "sharedlocation.h"
#include "transition.h"
#include "transportque.h"
#include "units.h"
#include "vattachmentpoint.h"
#include "vedit.h"
#include "vframe.h"
#include "vmodule.h"
#include "vplugin.h"
#include "vtrack.h"
#include <string.h>

VModule::VModule(RenderEngine *renderengine, 
	CommonRender *commonrender, 
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_VIDEO;
	input_temp = transition_temp = 0;
	overlayer = 0;
}

VModule::~VModule()
{
	if(input_temp) 
	{
		delete input_temp;
	}
	if(transition_temp) 
	{
		delete transition_temp;
	}
	if(overlayer) delete overlayer;
}


AttachmentPoint* VModule::new_attachment(Plugin *plugin)
{
	return new VAttachmentPoint(renderengine, plugin);
}

int VModule::get_buffer_size()
{
	return 1;
}

CICache* VModule::get_cache()
{
	if(renderengine) 
		return renderengine->get_vcache();
	else
		return cache;
}

int VModule::import_frame(VFrame *output,
	VEdit *current_edit,
	int64_t input_position,
	int direction)
{
	int64_t corrected_position;

// Translation of edit
	float in_x1;
	float in_y1;
	float in_w1;
	float in_h1;
	float out_x1;
	float out_y1;
	float out_w1;
	float out_h1;
	int result = 0;

	corrected_position = input_position;
	if(direction == PLAY_REVERSE) corrected_position--;

// Load frame into output
	if(current_edit &&
		current_edit->asset)
	{
		File *source = get_cache()->check_out(current_edit->asset);

		if(source)
		{
			source->set_video_position(corrected_position - 
				current_edit->startproject + 
				current_edit->startsource,
				get_edl()->session->frame_rate);
			source->set_layer(current_edit->channel);

			((VTrack*)track)->calculate_input_transfer(current_edit->asset, 
				input_position, 
				direction, 
				in_x1, 
				in_y1, 
				in_w1, 
				in_h1,
				out_x1, 
				out_y1, 
				out_w1, 
				out_h1);



// file -> temp -> output
			if( !EQUIV(in_x1, 0) || 
				!EQUIV(in_y1, 0) || 
				!EQUIV(in_w1, track->track_w) || 
				!EQUIV(in_h1, track->track_h) || 
				!EQUIV(out_x1, 0) ||
				!EQUIV(out_y1, 0) ||
				!EQUIV(out_w1, track->track_w) ||
				!EQUIV(out_h1, track->track_h) ||
				!EQUIV(in_w1, current_edit->asset->width) ||
				!EQUIV(in_h1, current_edit->asset->height))
			{
// Fix buffers
				if(input_temp && 
					(input_temp->get_w() != current_edit->asset->width ||
					input_temp->get_h() != current_edit->asset->height))
				{
					delete input_temp;
					input_temp = 0;
				}





				if(!input_temp)
				{
//printf("VModule::import_frame 4\n");
					input_temp = new VFrame(0,
						current_edit->asset->width,
						current_edit->asset->height,
						get_edl()->session->color_model,
						-1);
				}
//printf("VModule::import_frame 5\n");

// file -> temp
				result = source->read_frame(input_temp);
				if(!overlayer)
				{
					overlayer = new OverlayFrame(renderengine->preferences->processors);
				}
// printf("VModule::import_frame 1 %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", 
// 	in_x1, 
// 	in_y1, 
// 	in_w1, 
// 	in_h1, 
// 	out_x1, 
// 	out_y1, 
// 	out_w1, 
// 	out_h1);

// temp -> output
// for(int j = 0; j < output->get_w() * 3 * 5; j++)
// 	output->get_rows()[0][j] = 255;

				output->clear_frame();


// get_cache()->check_in(current_edit->asset);
// return;

// TRANSFER_REPLACE is the fastest transfer mode but it has the disadvantage
// of producing green borders in floating point translation of YUV
				int mode = TRANSFER_REPLACE;
				if(get_edl()->session->interpolation_type != NEAREST_NEIGHBOR &&
					cmodel_is_yuv(output->get_color_model()))
					mode = TRANSFER_NORMAL;

				overlayer->overlay(output,
					input_temp, 
					in_x1,
					in_y1,
					in_x1 + in_w1,
					in_y1 + in_h1,
					out_x1,
					out_y1,
					out_x1 + out_w1,
					out_y1 + out_h1,
					1,
					mode,
					get_edl()->session->interpolation_type);
			}
			else
// file -> output
			{

//printf("VModule::import_frame 6\n");
				result = source->read_frame(output);
			}
//printf("VModule::import_frame 6\n");

			get_cache()->check_in(current_edit->asset);
		}
		else
		{
			output->clear_frame();
			result = 1;
		}
	}
	else
// Silence
	{
		output->clear_frame();
	}
//printf("VModule::import_frame 7\n");

	return result;
}



int VModule::render(VFrame *output,
	int64_t input_position,
	int direction,
	int use_nudge)
{
	int result = 0;
	if(use_nudge) input_position += track->nudge;

	update_transition(input_position, direction);

	VEdit* current_edit = (VEdit*)track->edits->editof(input_position, 
		direction,
		0);
	VEdit* previous_edit = 0;

	if(!current_edit) 
	{
		output->clear_frame();
		return 0;
	}


// Process transition
	if(transition)
	{
// Load incoming frame
		if(!transition_temp)
		{
			transition_temp = new VFrame(0,
				track->track_w,
				track->track_h,
				get_edl()->session->color_model,
				-1);
		}

		result = import_frame(transition_temp, 
			current_edit, 
			input_position,
			direction);


// Load transition buffer
		previous_edit = (VEdit*)current_edit->previous;

		result |= import_frame(output, 
			previous_edit, 
			input_position,
			direction);

// Execute plugin with transition_temp and output here
		int64_t corrected_position = input_position;
		if(direction == PLAY_REVERSE)
			corrected_position--;

		transition_server->process_realtime(&transition_temp, 
			&output,
			(direction == PLAY_FORWARD) ? 
				(input_position - current_edit->startproject) :
				(input_position - current_edit->startproject - 1),
			transition->length);
	}
	else
	{
// Load output buffer
		result = import_frame(output, 
			current_edit, 
			input_position,
			direction);
	}


	return result;
}






void VModule::create_objects()
{
	Module::create_objects();
}






