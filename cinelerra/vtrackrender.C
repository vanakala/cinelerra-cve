
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
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "fadeengine.h"
#include "file.h"
#include "floatautos.h"
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
