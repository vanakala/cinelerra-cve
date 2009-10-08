
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

#include "asset.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "commonrender.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "patch.h"
#include "pluginarray.h"
#include "preferences.h"
#include "renderengine.h"
#include "sharedlocation.h"
#include "transition.h"
#include "transportque.h"
#include "units.h"
#include "vattachmentpoint.h"
#include "vdevicex11.h"
#include "vedit.h"
#include "vframe.h"
#include "videodevice.h"
#include "vmodule.h"
#include "vrender.h"
#include "vplugin.h"
#include "vtrack.h"
#include <string.h>
#include "interlacemodes.h"
#include "maskengine.h"
#include "automation.h"

VModule::VModule(RenderEngine *renderengine, 
	CommonRender *commonrender, 
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_VIDEO;
	overlay_temp = 0;
	input_temp = 0;
	transition_temp = 0;
	if (renderengine)
		masker = new MaskEngine(renderengine->preferences->processors);
	else
		masker = new MaskEngine(plugin_array->mwindow->preferences->processors);
	
}

VModule::~VModule()
{
	if(overlay_temp) delete overlay_temp;
	if(input_temp) delete input_temp;
	if(transition_temp) delete transition_temp;
	delete masker;
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
	double frame_rate,
	int direction,
	int use_opengl)
{
	int64_t corrected_position;
	int64_t corrected_position_project;
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
	double edl_rate = get_edl()->session->frame_rate;
	int64_t input_position_project = (int64_t)(input_position * 
		edl_rate / 
		frame_rate + 
		0.001);
	if(!output) printf("VModule::import_frame 10 output=%p\n", output);

	corrected_position = input_position;
	corrected_position_project = input_position_project;
	if(direction == PLAY_REVERSE)
	{
		corrected_position--;
		input_position_project--;
	}

	VDeviceX11 *x11_device = 0;
	if(use_opengl)
	{
		if(renderengine && renderengine->video)
		{
			x11_device = (VDeviceX11*)renderengine->video->get_output_base();
			output->set_opengl_state(VFrame::RAM);
		}
	}


// Load frame into output
	if(current_edit &&
		current_edit->asset)
	{
		get_cache()->age();
		File *source = get_cache()->check_out(current_edit->asset,
			get_edl());
//		get_cache()->dump();

		if(source)
		{
			int64_t edit_startproject = (int64_t)(current_edit->startproject * 
				frame_rate / 
				edl_rate);
			int64_t edit_startsource = (int64_t)(current_edit->startsource *
				frame_rate /
				edl_rate);
			uint64_t position = corrected_position - 
				edit_startproject + 
				edit_startsource;
			// if we hit the end of stream, freeze at last frame
			uint64_t max_position = source->get_video_length(frame_rate) - 1;
			if (position > max_position) position = max_position;
			int use_cache = renderengine && 
				renderengine->command->single_frame();
			int use_asynchronous = !use_cache && renderengine &&
				renderengine->command->realtime &&
				renderengine->edl->session->video_asynchronous;

			if(use_asynchronous)
				source->start_video_decode_thread();
			else
				source->stop_video_thread();

			source->set_video_position(position, frame_rate);
			source->set_layer(current_edit->channel);

			((VTrack*)track)->calculate_input_transfer(current_edit->asset, 
				input_position_project, 
				direction, 
				in_x1, 
				in_y1, 
				in_w1, 
				in_h1,
				out_x1, 
				out_y1, 
				out_w1, 
				out_h1);

//
//			printf("VModule::import_frame 1 [ilace] Project: mode (%d) Asset: autofixoption (%d), mode (%d), method (%d)\n", 
//			get_edl()->session->interlace_mode,
//			current_edit->asset->interlace_autofixoption,
//			current_edit->asset->interlace_mode,
//			current_edit->asset->interlace_fixmethod);

			// Determine the interlacing method to use.
			int interlace_fixmethod = ilaceautofixmethod2(get_edl()->session->interlace_mode,
					current_edit->asset->interlace_autofixoption,
					current_edit->asset->interlace_mode,
					current_edit->asset->interlace_fixmethod);
//
//			char string[BCTEXTLEN];
//			ilacefixmethod_to_text(string,interlace_fixmethod);
//			printf("VModule::import_frame 1 [ilace] Compensating by using: '%s'\n",string);

			// Compensate for the said interlacing...
			switch (interlace_fixmethod) {
				case BC_ILACE_FIXMETHOD_NONE:
				
				break;
				case BC_ILACE_FIXMETHOD_UPONE:
					out_y1--;
				break;
				case BC_ILACE_FIXMETHOD_DOWNONE:
					out_y1++;
				break;
				default:
					printf("vmodule::importframe WARNING - unknown fix method for interlacing, no compensation in effect\n");
			}


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




// Get temporary input buffer
				VFrame **input = 0;
// Realtime playback
				if(commonrender)
				{
					VRender *vrender = (VRender*)commonrender;
					input = &vrender->input_temp;
				}
				else
// Menu effect
				{
					input = &input_temp;
				}


				if((*input) && 
					((*input)->get_w() != current_edit->asset->width ||
					(*input)->get_h() != current_edit->asset->height))
				{
					delete (*input);
					(*input) = 0;
				}





				if(!(*input))
				{
					(*input) = new VFrame(0,
						current_edit->asset->width,
						current_edit->asset->height,
						get_edl()->session->color_model,
						-1);
				}



				(*input)->copy_stacks(output);

// file -> temp
// Cache for single frame only
				if(use_cache) source->set_cache_frames(1);
				result = source->read_frame((*input));
				if(use_cache) source->set_cache_frames(0);
				(*input)->set_opengl_state(VFrame::RAM);

//printf("VModule::import_frame 1 %lld %f\n", input_position, frame_rate);

// Find an overlayer object to perform the camera transformation
				OverlayFrame *overlayer = 0;

// OpenGL playback uses hardware
				if(use_opengl)
				{
				}
				else
// Realtime playback
				if(commonrender)
				{
					VRender *vrender = (VRender*)commonrender;
					overlayer = vrender->overlayer;
				}
				else
// Menu effect
				{
					if(!plugin_array)
						printf("VModule::import_frame neither plugin_array nor commonrender is defined.\n");
					if(!overlay_temp)
					{
						overlay_temp = new OverlayFrame(plugin_array->mwindow->preferences->processors);
					}

					overlayer = overlay_temp;
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

				if(use_opengl)
				{
					x11_device->do_camera(output,
						(*input), 
						in_x1,
						in_y1,
						in_x1 + in_w1,
						in_y1 + in_h1,
						out_x1,
						out_y1,
						out_x1 + out_w1,
						out_y1 + out_h1);
				}
				else
				{
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
						(*input), 
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
				result = 1;
				output->copy_stacks((*input));
			}
			else
// file -> output
			{
// Cache single frames only
				if(use_cache) source->set_cache_frames(1);
				result = source->read_frame(output);
				if(use_cache) source->set_cache_frames(0);
				output->set_opengl_state(VFrame::RAM);
			}

			get_cache()->check_in(current_edit->asset);
		}
		else
		{
			if(use_opengl)
			{
				x11_device->clear_input(output);
			}
			else
			{
				output->clear_frame();
			}
			result = 1;
		}
	}
	else
// Silence
	{
		if(use_opengl)
		{
			x11_device->clear_input(output);
		}
		else
		{
			output->clear_frame();
		}
	}


	return result;
}



int VModule::render(VFrame *output,
	int64_t start_position,
	int direction,
	double frame_rate,
	int use_nudge,
	int debug_render,
	int use_opengl)
{
	int result = 0;
	double edl_rate = get_edl()->session->frame_rate;

	if(use_nudge) start_position += (int64_t)(track->nudge * 
		frame_rate / 
		edl_rate);

	int64_t start_position_project = (int64_t)(start_position *
		edl_rate /
		frame_rate + 
		0.5);

	update_transition(start_position_project, 
		direction);

	VEdit* current_edit = (VEdit*)track->edits->editof(start_position_project, 
		direction,
		0);
	VEdit* previous_edit = 0;

	if(debug_render)
		printf("    VModule::render %d %lld %s transition=%p opengl=%d current_edit=%p output=%p\n", 
			use_nudge, 
			start_position_project,
			track->title,
			transition,
			use_opengl,
			current_edit,
			output);

	if(!current_edit)
	{
		output->clear_frame();
		// We do not apply mask here, since alpha is 0, and neither substracting nor multypling changes it
		// Another mask mode - "addition" should be added to be able to create mask from empty frames
		// in this case we would call masking here too...
		return 0;
	}




// Process transition
	if(transition && transition->on)
	{

// Get temporary buffer
		VFrame **transition_input = 0;
		if(commonrender)
		{
			VRender *vrender = (VRender*)commonrender;
			transition_input = &vrender->transition_temp;
		}
		else
		{
			transition_input = &transition_temp;
		}

		if((*transition_input) &&
			((*transition_input)->get_w() != track->track_w ||
			(*transition_input)->get_h() != track->track_h))
		{
			delete (*transition_input);
			(*transition_input) = 0;
		}

// Load incoming frame
		if(!(*transition_input))
		{
			(*transition_input) = new VFrame(0,
				track->track_w,
				track->track_h,
				get_edl()->session->color_model,
				-1);
		}

		result = import_frame((*transition_input), 
			current_edit, 
			start_position,
			frame_rate,
			direction,
			use_opengl);


// Load transition buffer
		previous_edit = (VEdit*)current_edit->previous;

		result |= import_frame(output, 
			previous_edit, 
			start_position,
			frame_rate,
			direction,
			use_opengl);

// Execute plugin with transition_input and output here
		if(renderengine) 
			transition_server->set_use_opengl(use_opengl, renderengine->video);
		transition_server->process_transition((*transition_input), 
			output,
			(direction == PLAY_FORWARD) ? 
				(start_position_project - current_edit->startproject) :
				(start_position_project - current_edit->startproject - 1),
			transition->length);
	}
	else
	{
// Load output buffer
		result = import_frame(output, 
			current_edit, 
			start_position,
			frame_rate,
			direction,
			use_opengl);
	}
	
	int64_t mask_position;
	if (renderengine)
		mask_position = renderengine->vrender->current_position;
	else 
		mask_position = start_position;
	masker->do_mask(output, 
		mask_position,
		edl_rate,
		edl_rate,
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK], 
		direction,
		1);      // we are calling before plugins


	return result;
}






void VModule::create_objects()
{
	Module::create_objects();
}






