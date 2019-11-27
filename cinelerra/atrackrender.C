
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
#include "audiorender.inc"

ATrackRender::ATrackRender(Track *track, AudioRender *audiorender)
 : TrackRender(track)
{
	arender = audiorender;
	num_frames = 0;
	for(int i =  0; i < MAXCHANNELS; i++)
		aframes[i] = 0;
	module_levels = new LevelHistory();
}

ATrackRender::~ATrackRender()
{
	for(int i =  0; i < MAXCHANNELS; i++)
		delete aframes[i];
	delete module_levels;
}

AFrame *ATrackRender::read_aframe(AFrame *aframe, Edit *edit, int filenum)
{
	ptstime pts = aframe->pts;
	ptstime duration = aframe->get_source_duration();
	ptstime sample_error = 0.8 / aframe->samplerate;

	File *file = media_file(edit, filenum);

	if(!file)
	{
		aframe->clear_buffer();
		aframe->set_filled_length();
		return 0;
	}

	aframe->source_length = 0;
	aframe->source_pts = pts - edit->get_pts() + edit->get_source_pts();
	file->get_samples(aframe);
	return aframe;
}

AFrame **ATrackRender::get_aframes(AFrame **output, int out_channels)
{
	ptstime pts = output[0]->pts;
	ptstime duration = output[0]->source_duration;
	Edit *edit = media_track->editof(pts);
	int channels, buflen;

	if(is_playable(pts, edit))
	{
		channels = edit->asset->channels;
		buflen = output[0]->buffer_length;

		if(num_frames < channels)
		{
			for(int i = 0; i < channels; i++)
			{
				if(i < num_frames)
					aframes[i]->check_buffer(buflen);
				else
					aframes[i] = new AFrame(buflen);
				aframes[i]->channel = i;
			}
			num_frames = channels;
		}
// siin tuleb kontrollida edit-i, plugina, autot
		for(int i = 0; i < num_frames; i++)
		{
			aframes[i]->reset_buffer();
			aframes[i]->set_fill_request(pts, duration);
			read_aframe(aframes[i], edit, 0);
		}

		for(int i = 0; i < out_channels && i < num_frames; i++)
			output[i]->copy(aframes[i]);
	}
	return output;
}

AFrame *ATrackRender::get_aframe(AFrame *buffer)
{
}
