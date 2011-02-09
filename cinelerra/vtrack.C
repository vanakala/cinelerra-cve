
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
#include "autoconf.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "datatype.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.inc"
#include "units.h"
#include "vautomation.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vmodule.h"
#include "vpluginset.h"
#include "vtrack.h"

VTrack::VTrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_VIDEO;
	draw = 1;
	one_unit = (ptstime)1.0 / edl->session->frame_rate;
}

VTrack::~VTrack()
{
}

void VTrack::create_objects()
{
	Track::create_objects();
	automation = new VAutomation(edl, this);
	automation->create_objects();
	edits = new VEdits(edl, this);
}

// Used by PlaybackEngine
void VTrack::synchronize_params(Track *track)
{
	Track::synchronize_params(track);

	VTrack *vtrack = (VTrack*)track;
}

// Used by EDL::operator=
int VTrack::copy_settings(Track *track)
{
	Track::copy_settings(track);

	VTrack *vtrack = (VTrack*)track;
	return 0;
}

int VTrack::vertical_span(Theme *theme)
{
	int track_h = Track::vertical_span(theme);
	int patch_h = 0;
	if(expand_view)
	{
		patch_h += theme->title_h + theme->play_h + theme->fade_h + theme->mode_h;
	}
	return MAX(track_h, patch_h);
}


PluginSet* VTrack::new_plugins()
{
	return new VPluginSet(edl, this);
}

int VTrack::load_defaults(BC_Hash *defaults)
{
	Track::load_defaults(defaults);
	return 0;
}

void VTrack::set_default_title()
{
	Track *current = ListItem<Track>::owner->first;
	int i;
	for(i = 0; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO) i++;
	}
	sprintf(title, _("Video %d"), i);
}

posnum VTrack::to_units(ptstime position, int round)
{
	if(round)
	{
		return Units::round(position * edl->session->frame_rate);
	}
	else
	{
// Kludge for rounding errors, just on a smaller scale than formal rounding
		position *= edl->session->frame_rate;
		return Units::to_int64(position);
	}
}

double VTrack::from_units(posnum position)
{
	return (double)position / edl->session->frame_rate;
}

void VTrack::save_header(FileXML *file)
{
	file->tag.set_property("TYPE", "VIDEO");
}

int VTrack::direct_copy_possible(ptstime start, int use_nudge)
{
	int i;
	if(use_nudge) start += nudge;

// Track size must equal output size
	if(track_w != edl->session->output_w || track_h != edl->session->output_h)
		return 0;
// No automation must be present in the track
	if(!automation->direct_copy_possible(start))
		return 0;
// No plugin must be present
	if(plugin_used(start))
		return 0;
// No transition
	if(get_current_transition(start))
		return 0;

	return 1;
}

void VTrack::calculate_input_transfer(Asset *asset, 
	ptstime position,
	float &in_x, 
	float &in_y, 
	float &in_w, 
	float &in_h,
	float &out_x, 
	float &out_y, 
	float &out_w, 
	float &out_h)
{
	float auto_x, auto_y, auto_z;
	float camera_z = 1;
	float camera_x = asset->width / 2;
	float camera_y = asset->height / 2;
// camera and output coords
	float z[6], x[6], y[6];

// get camera center in asset
	automation->get_camera(&auto_x, 
		&auto_y, 
		&auto_z, 
		position);
	camera_z *= auto_z;
	camera_x += auto_x;
	camera_y += auto_y;

// get camera coords on asset
	x[0] = camera_x - (float)track_w / 2 / camera_z;
	y[0] = camera_y - (float)track_h / 2 / camera_z;
	x[1] = x[0] + (float)track_w / camera_z;
	y[1] = y[0] + (float)track_h / camera_z;

// get asset coords on camera
	x[2] = 0;
	y[2] = 0;
	x[3] = track_w;
	y[3] = track_h;

// crop asset coords on camera
	if(x[0] < 0)
	{
		x[2] -= x[0] * camera_z;
		x[0] = 0;
	}
	if(y[0] < 0)
	{
		y[2] -= y[0] * camera_z;
		y[0] = 0;
	}
	if(x[1] > asset->width)
	{
		x[3] -= (x[1] - asset->width) * camera_z;
		x[1] = asset->width;
	}
	if(y[1] > asset->height)
	{
		y[3] -= (y[1] - asset->height) * camera_z;
		y[1] = asset->height;
	}

// get output bounding box
	out_x = x[2];
	out_y = y[2];
	out_w = x[3] - x[2];
	out_h = y[3] - y[2];

	in_x = x[0];
	in_y = y[0];
	in_w = x[1] - x[0];
	in_h = y[1] - y[0];
}

void VTrack::calculate_output_transfer(ptstime position, 
	float &in_x, 
	float &in_y, 
	float &in_w, 
	float &in_h,
	float &out_x, 
	float &out_y, 
	float &out_w, 
	float &out_h)
{
	float center_x, center_y, center_z;
	float x[4], y[4];
	x[0] = 0;
	y[0] = 0;
	x[1] = track_w;
	y[1] = track_h;

	automation->get_projector(&center_x, 
		&center_y, 
		&center_z, 
		position);

	center_x += edl->session->output_w / 2;
	center_y += edl->session->output_h / 2;

	x[2] = center_x - (track_w / 2) * center_z;
	y[2] = center_y - (track_h / 2) * center_z;
	x[3] = x[2] + track_w * center_z;
	y[3] = y[2] + track_h * center_z;

// Clip to boundaries of output
	if(x[2] < 0)
	{
		x[0] -= (x[2] - 0) / center_z;
		x[2] = 0;
	}
	if(y[2] < 0)
	{
		y[0] -= (y[2] - 0) / center_z;
		y[2] = 0;
	}
	if(x[3] > edl->session->output_w)
	{
		x[1] -= (x[3] - edl->session->output_w) / center_z;
		x[3] = edl->session->output_w;
	}
	if(y[3] > edl->session->output_h)
	{
		y[1] -= (y[3] - edl->session->output_h) / center_z;
		y[3] = edl->session->output_h;
	}

	in_x = x[0];
	in_y = y[0];
	in_w = x[1] - x[0];
	in_h = y[1] - y[0];
	out_x = x[2];
	out_y = y[2];
	out_w = x[3] - x[2];
	out_h = y[3] - y[2];
}

void VTrack::translate(float offset_x, float offset_y, int do_camera)
{
	int subscript;
	if(do_camera) 
		subscript = AUTOMATION_CAMERA_X;
	else
		subscript = AUTOMATION_PROJECTOR_X;

// Translate default keyframe
	((FloatAuto*)automation->autos[subscript]->default_auto)->value += offset_x;
	((FloatAuto*)automation->autos[subscript + 1]->default_auto)->value += offset_y;

// Translate everyone else
	for(Auto *current = automation->autos[subscript]->first; 
		current; 
		current = NEXT)
	{
		((FloatAuto*)current)->value += offset_x;
	}

	for(Auto *current = automation->autos[subscript + 1]->first; 
		current; 
		current = NEXT)
	{
		((FloatAuto*)current)->value += offset_y;
	}
}
