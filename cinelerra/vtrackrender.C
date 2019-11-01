
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#include "automation.h"
#include "clip.h"
#include "cropautos.h"
#include "cropengine.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "fadeengine.h"
#include "file.h"
#include "floatautos.h"
#include "maskauto.h"
#include "maskautos.h"
#include "maskengine.h"
#include "preferences.h"
#include "track.h"
#include "vtrackrender.h"
#include "vframe.h"

VTrackRender::VTrackRender(Track *track)
 : TrackRender(track)
{
	fader = 0;
}

VTrackRender::~VTrackRender()
{
	delete fader;
}

VFrame *VTrackRender::get_frame(VFrame *frame)
{
	ptstime pts = frame->get_pts();
	ptstime src_pts;
	Edit *edit = track->editof(pts);
	File *file = media_file(edit);

	frame->set_layer(edit->channel);

	if(is_playable(pts, edit) && file)
	{
		src_pts = pts - edit->get_pts() + edit->get_source_pts();
		frame->set_source_pts(src_pts);
		file->get_frame(frame);
		render_fade(frame);
		render_mask(frame);
		render_crop(frame);
	}
	else
	{
		frame->clear_frame();
		frame->set_duration(track->edl->this_edlsession->frame_duration());
	}
	return frame;
}

void VTrackRender::render_fade(VFrame *frame)
{
	double value;
	FloatAuto *prev, *next;

	prev = next = 0;
	value = ((FloatAutos*)track->automation->autos[AUTOMATION_FADE])->get_value(frame->get_pts(), prev, next);

	CLAMP(value, 0, 100);

	value /= 100;

	if(!EQUIV(value, 1.0))
	{
		if(!fader)
			fader = new FadeEngine(preferences_global->processors);
		fader->do_fade(frame, value);
	}
}

void VTrackRender::render_mask(VFrame *frame)
{
	int total_points = 0;
	Auto *current = 0;
	MaskAutos *keyframe_set =
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK];
	MaskAuto *keyframe = (MaskAuto*)keyframe_set->get_prev_auto(frame->get_pts(),
		current);

	if(!keyframe)
		return;

	for(int i = 0; i < keyframe->masks.total; i++)
	{
		int submask_points = keyframe->get_submask(i)->points.total;

		if(submask_points > 1)
			total_points += submask_points;
	}

// Ignore masks that are not relevant
	if(total_points <= 2 || (keyframe->value == 0 &&
			(keyframe_set->get_mode() == MASK_SUBTRACT_ALPHA ||
			keyframe_set->get_mode() == MASK_SUBTRACT_COLOR)))
		return;

// Fake certain masks
	if(keyframe->value == 0 &&
		keyframe_set->get_mode() == MASK_MULTIPLY_COLOR)
	{
		frame->clear_frame();
		return;
	}
	if(!masker)
		masker = new MaskEngine(preferences_global->processors);

	masker->do_mask(frame, keyframe_set, 0);
}

void VTrackRender::render_crop(VFrame *frame)
{
	int left, right, top, bottom;
	CropAutos *cropautos = (CropAutos *)track->automation->autos[AUTOMATION_CROP];

	if(!cropautos->first)
		return;

	cropautos->get_values(frame->get_pts(),
		&left, &right, &top, &bottom);

	if(left == 0 && top == 0 && right >= frame->get_w() && bottom >= frame->get_h())
		return;

	if(!cropper)
		cropper = new CropEngine();

	frame = cropper->do_crop(cropautos, frame, 0);
}
