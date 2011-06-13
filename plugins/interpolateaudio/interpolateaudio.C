
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "guicast.h"
#include "filexml.h"
#include "language.h"
#include "mainprogress.h"
#include "pluginaclient.h"
#include "transportque.h"

#include <string.h>


class InterpolateAudioEffect : public PluginAClient
{
public:
	InterpolateAudioEffect(PluginServer *server);
	~InterpolateAudioEffect();

	PLUGIN_CLASS_MEMBERS_TRANSITION

	void process_frame(AFrame *buffer);
	int is_realtime();
	int has_pts_api();
	int uses_gui();

#define FRAGMENT_SIZE 4096
	AFrame *start_frame;
	AFrame *end_frame;
	double start_sample;
	double end_sample;
	ptstime range_start;
	ptstime range_end;
};


REGISTER_PLUGIN(InterpolateAudioEffect)

InterpolateAudioEffect::InterpolateAudioEffect(PluginServer *server)
 : PluginAClient(server)
{
	start_frame = 0;
	end_frame = 0;
}

InterpolateAudioEffect::~InterpolateAudioEffect()
{
	if(start_frame) delete start_frame;
	if(end_frame) delete end_frame;
}


const char* InterpolateAudioEffect::plugin_title()
{
	return N_("Interpolate");
}


int InterpolateAudioEffect::is_realtime()
{
	return 1;
}

int InterpolateAudioEffect::uses_gui()
{
	return 0;
}

int InterpolateAudioEffect::has_pts_api()
{
	return 1;
}

#include "picon_png.h"
NEW_PICON_MACRO(InterpolateAudioEffect)

void InterpolateAudioEffect::process_frame(AFrame *aframe)
{
	double slope;
	double intercept;

	if(!start_frame) start_frame = new AFrame(FRAGMENT_SIZE);
	if(!end_frame) end_frame = new AFrame(FRAGMENT_SIZE);

// On first sample of range.  Get boundary samples of effect.
	if(PTSEQU(aframe->pts, source_start_pts))
	{
// Need to read before desired sample to diffuse transients after audio
// seeks.
		range_start = source_start_pts;
		range_end = source_start_pts + total_len_pts;
		start_frame->copy_pts(aframe);
		start_frame->pts -= (ptstime)FRAGMENT_SIZE / start_frame->samplerate;
		start_frame->source_length = FRAGMENT_SIZE;
		get_aframe_rt(start_frame);
		start_sample = start_frame->buffer[FRAGMENT_SIZE - 1];

		end_frame->copy_pts(aframe);
		end_frame->pts = range_end - (ptstime)FRAGMENT_SIZE / end_frame->samplerate;
		end_frame->source_length = FRAGMENT_SIZE;
		get_aframe_rt(end_frame);
		end_sample = end_frame->buffer[FRAGMENT_SIZE - 1];
	}

	aframe->set_filled(aframe->source_length);

	for(int i = 0; i < aframe->length; i++)
	{
		double end_fraction = (((double)i / aframe->samplerate) + aframe->pts - range_start) / 
			(range_end - range_start);
		double start_fraction = 1.0 - end_fraction;
		double out_sample = start_sample * start_fraction + 
			end_sample * end_fraction;
		aframe->buffer[i] = out_sample;
	}
}


