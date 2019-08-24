
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
#include "automation.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "cache.h"
#include "clip.h"
#include "commonrender.h"
#include "cropengine.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "formatpresets.h"
#include "file.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "plugin.h"
#include "pluginarray.h"
#include "preferences.h"
#include "renderengine.h"
#include "tmpframecache.h"
#include "transportcommand.h"
#include "units.h"
#include "vattachmentpoint.h"
#include "vframe.h"
#include "vmodule.h"
#include "vrender.h"
#include "vtrack.h"
#include "interlacemodes.h"
#include "maskengine.h"

#include <string.h>

VModule::VModule(RenderEngine *renderengine, 
	CommonRender *commonrender, 
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_VIDEO;
	overlay_temp = 0;
	if (renderengine)
		masker = new MaskEngine(renderengine->preferences->processors);
	else
		masker = new MaskEngine(plugin_array->mwindow->preferences->processors);
	cropper = new CropEngine();
}

VModule::~VModule()
{
	if(overlay_temp) delete overlay_temp;
	delete masker;
	delete cropper;
}

AttachmentPoint* VModule::new_attachment(Plugin *plugin)
{
	return new VAttachmentPoint(renderengine, plugin);
}

CICache* VModule::get_cache()
{
	if(renderengine) 
		return renderengine->get_vcache();
	else
		return cache;
}

VFrame *VModule::import_frame(VFrame *output,
	Edit *current_edit)
{
// Translation of edit
	int in_x1;
	int in_y1;
	int in_w1;
	int in_h1;
	int out_x1;
	int out_y1;
	int out_w1;
	int out_h1;

// Load frame into output
	if(current_edit &&
		current_edit->asset)
	{
		get_cache()->age();
		File *source = get_cache()->check_out(current_edit->asset,
			get_edl());

		if(source)
		{
			ptstime src_pts = output->get_pts() -
				current_edit->get_pts() +
				current_edit->get_source_pts();

			// if we hit the end of stream, freeze at last frame
			ptstime max_pts = current_edit->asset->video_duration;
			if(src_pts > max_pts)
				src_pts = max_pts;

			output->set_source_pts(src_pts);
			output->set_layer(current_edit->channel);
			source->get_frame(output);

			get_cache()->check_in(current_edit->asset);
		}
		else
		{
			output->clear_frame();
		}
	}
	else
// Silence
	{
		output->clear_frame();
	}
	return output;
}

VFrame *VModule::render(VFrame *output)
{
	update_transition(output->get_pts());
	Edit* current_edit = track->editof(output->get_pts());
	Edit* previous_edit = 0;

	if(!current_edit)
	{
		output->clear_frame();
		// We do not apply mask here, since alpha is 0, and neither substracting nor multypling changes it
		// Another mask mode - "addition" should be added to be able to create mask from empty frames
		// in this case we would call masking here too...
		return output;
	}

// Process transition
	if(transition && transition->on)
	{
// Get temporary buffer
		VFrame *transition_temp = BC_Resources::tmpframes.get_tmpframe(
			track->track_w, track->track_h,
			edlsession->color_model);

		transition_temp->copy_pts(output);
		transition_temp = import_frame(transition_temp,
			current_edit);

// Load transition buffer
		previous_edit = current_edit->previous;

		output = import_frame(output, previous_edit);

// Execute plugin with transition_input and output here
		transition_server->process_transition(transition_temp,
			output,
			output->get_pts() - current_edit->get_pts(),
			transition->get_length());
		BC_Resources::tmpframes.release_frame(transition_temp);
	}
	else
	{
// Load output buffer
		output = import_frame(output, current_edit);
	}
	masker->do_mask(output, 
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK], 
		1);      // we are calling before plugins

	output = cropper->do_crop(track->automation->autos[AUTOMATION_CROP],
		output, 1);
	return output;
}
