// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "automation.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "clip.h"
#include "datatype.h"
#include "edl.inc"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "filexml.h"
#include "language.h"
#include "theme.h"
#include "tracks.inc"
#include "units.h"
#include "vtrack.h"

VTrack::VTrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_VIDEO;
	draw = 1;
	one_unit = (ptstime)1.0 / edlsession->frame_rate;
	automation = new Automation(edl, this);
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
		return Units::round(position * edlsession->frame_rate);
	}
	else
	{
// Kludge for rounding errors, just on a smaller scale than formal rounding
		position *= edlsession->frame_rate;
		return Units::to_int64(position);
	}
}

ptstime VTrack::from_units(posnum position)
{
	return (ptstime)position / edlsession->frame_rate;
}

void VTrack::save_header(FileXML *file)
{
	file->tag.set_property("TYPE", "VIDEO");
}

void VTrack::calculate_output_transfer(ptstime position, 
	int *in_x,
	int *in_y,
	int *in_w,
	int *in_h,
	int *out_x,
	int *out_y,
	int *out_w,
	int *out_h)
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

	center_x += edlsession->output_w / 2;
	center_y += edlsession->output_h / 2;

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
	if(x[3] > edlsession->output_w)
	{
		x[1] -= (x[3] - edlsession->output_w) / center_z;
		x[3] = edlsession->output_w;
	}
	if(y[3] > edlsession->output_h)
	{
		y[1] -= (y[3] - edlsession->output_h) / center_z;
		y[3] = edlsession->output_h;
	}

	*in_x = round(x[0]);
	*in_y = round(y[0]);
	*in_w = round(x[1] - x[0]);
	*in_h = round(y[1] - y[0]);
	*out_x = round(x[2]);
	*out_y = round(y[2]);
	*out_w = round(x[3] - x[2]);
	*out_h = round(y[3] - y[2]);
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

// Translate everyone else
	for(Auto *current = automation->get_auto_for_editing(0, subscript_x);
		current;
		current = NEXT)
	{
		((FloatAuto*)current)->add_value(offset_x);
	}

	for(Auto *current = automation->get_auto_for_editing(0, subscript_y);
		current;
		current = NEXT)
	{
		((FloatAuto*)current)->add_value(offset_y);
	}
}
