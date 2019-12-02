
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

#include "aframe.h"
#include "asset.h"
#include "atrackrender.h"
#include "bcsignals.h"
#include "edit.h"
#include "file.h"
#include "levelhist.h"
#include "plugin.inc"
#include "track.h"
#include "audiorender.h"

ATrackRender::ATrackRender(Track *track, AudioRender *audiorender)
 : TrackRender(track)
{
	arender = audiorender;
	module_levels = new LevelHistory();
}

ATrackRender::~ATrackRender()
{
	delete module_levels;
}

AFrame *ATrackRender::read_aframe(AFrame *aframe, Edit *edit, int filenum)
{
	ptstime sample_error = 0.8 / aframe->samplerate;
	AFrame *result;

	if(!(result = arender->get_file_frame(aframe->pts, aframe->source_duration,
		edit, filenum)))
	{
		if(!track_frame)
			track_frame = new AFrame(aframe->buffer_length);
		else
			track_frame->check_buffer(aframe->buffer_length);
		track_frame->copy_pts(aframe);
		track_frame->clear_buffer();
		track_frame->set_filled(aframe->source_duration);
		track_frame->channel = edit->channel;
		return 0;
	}

	return result;
}

AFrame **ATrackRender::get_aframes(AFrame **output, int out_channels)
{
	AFrame *aframe;
	ptstime pts = output[0]->pts;
	Edit *edit = media_track->editof(pts);

	if(is_playable(pts, edit))
	{
		aframe = read_aframe(output[0], edit, 0);
		for(int i = 0; i < out_channels; i++)
		{
			if(output[i]->channel == aframe->channel)
			{
				output[i]->copy(aframe);
				break;
			}
		}
	}
	return output;
}

AFrame *ATrackRender::get_aframe(AFrame *buffer)
{
}
