
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
#include "bcresources.h"
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
#include "units.h"
#include "vautomation.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vmodule.h"
#include "vtrack.h"

VTrack::VTrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_VIDEO;
	draw = 1;
	one_unit = (ptstime)1.0 / edl->session->frame_rate;
	automation = new VAutomation(edl, this);
	edits = new VEdits(edl, this);
}

// Used by PlaybackEngine
void VTrack::synchronize_params(Track *track)
{
	Track::synchronize_params(track);

	VTrack *vtrack = (VTrack*)track;
}

// Used by EDL::operator=
void VTrack::copy_settings(Track *track)
{
	Track::copy_settings(track);

	VTrack *vtrack = (VTrack*)track;
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

void VTrack::set_default_title()
{
	Track *current = ListItem<Track>::owner->first;
	int i;
	for(i = 0; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO) i++;
	}
	sprintf(title, _("Video %d"), i);
	BC_Resources::encode_to_utf8(title, BCTEXTLEN);
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

ptstime VTrack::from_units(posnum position)
{
	return (ptstime)position / edl->session->frame_rate;
}

void VTrack::save_header(FileXML *file)
{
	file->tag.set_property("TYPE", "VIDEO");
}

void VTrack::calculate_input_transfer(Asset *asset, 
	ptstime position,
	int *in_x,
	int *in_y,
	int *in_w,
	int *in_h,
	int *out_x,
	int *out_y,
	int *out_w,
	int *out_h)
{
	double auto_x, auto_y, auto_z;
	double camera_z = 1;
	double camera_x = asset->width / 2;
	double camera_y = asset->height / 2;
// camera and output coords
	double z[6], x[6], y[6];

// get camera center in asset
	automation->get_camera(&auto_x, 
		&auto_y, 
		&auto_z, 
		position);
	camera_z *= auto_z;
	camera_x += auto_x;
	camera_y += auto_y;

// get camera coords on asset
	x[0] = camera_x - (double)track_w / 2 / camera_z;
	y[0] = camera_y - (double)track_h / 2 / camera_z;
	x[1] = x[0] + (double)track_w / camera_z;
	y[1] = y[0] + (double)track_h / camera_z;

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
	*out_x = round(x[2]);
	*out_y = round(y[2]);
	*out_w = round(x[3] - x[2]);
	*out_h = round(y[3] - y[2]);

	*in_x = round(x[0]);
	*in_y = round(y[0]);
	*in_w = round(x[1] - x[0]);
	*in_h = round(y[1] - y[0]);
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
	double center_x, center_y, center_z;
	double x[4], y[4];
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
	int subscript_x, subscript_y;

	if(do_camera)
	{
		subscript_x = AUTOMATION_CAMERA_X;
		subscript_y = AUTOMATION_CAMERA_Y;
	}
	else
	{
		subscript_x = AUTOMATION_PROJECTOR_X;
		subscript_y = AUTOMATION_PROJECTOR_Y;
	}

	if(!automation->autos[subscript_x]->first)
		((FloatAutos*)automation->autos[subscript_x])->insert_auto(0);
// Translate everyone else
	for(Auto *current = automation->autos[subscript_x]->first;
		current; 
		current = NEXT)
	{
		((FloatAuto*)current)->add_value(offset_x);
	}

	if(!automation->autos[subscript_y]->first)
		((FloatAutos*)automation->autos[subscript_y])->insert_auto(0);

	for(Auto *current = automation->autos[subscript_y]->first;
		current; 
		current = NEXT)
	{
		((FloatAuto*)current)->add_value(offset_y);
	}
}
