// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "bcdisplayinfo.h"
#include "bchash.h"
#include "language.h"
#include "normalize.h"
#include "normalizewindow.h"
#include "picon_png.h"
#include "units.h"

#include <string.h>

REGISTER_PLUGIN


NormalizeMain::NormalizeMain(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	memset(peak, 0, sizeof(double) * MAXCHANNELS);
	writing = 0;
}

NormalizeMain::~NormalizeMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void NormalizeMain::load_defaults()
{
	defaults = load_defaults_file("normalize.rc");

	db_over = defaults->get("DBOVER", 0);
	separate_tracks = defaults->get("SEPERATE_TRACKS", 1);
}

void NormalizeMain::save_defaults()
{
	defaults->update("DBOVER", db_over);
	defaults->update("SEPERATE_TRACKS", separate_tracks);
	defaults->save();
}

int NormalizeMain::process_loop(AFrame **aframes)
{
	int result = 0;
	int fragment_len;
	int duration_valid = 0;
	ptstime start_pts = get_start();
	ptstime end_pts = get_end();

	if(!writing)
	{
		AFrame *tmpframes[MAXCHANNELS];

		for(int j = 0; j < total_in_buffers; j++)
		{
			tmpframes[j] = clone_aframe(aframes[j]);
			tmpframes[j]->set_pts(start_pts);
		}
// Get peak
		for(ptstime current_pts = start_pts; current_pts < end_pts;)
		{
			fragment_len = tmpframes[0]->get_buffer_length();

			if(tmpframes[0]->get_pts() + tmpframes[0]->to_duration(fragment_len) > end_pts)
				fragment_len = tmpframes[0]->to_samples(end_pts - current_pts);

			for(int j = 0; j < total_in_buffers; j++)
			{
				tmpframes[j]->set_fill_request(current_pts, fragment_len);
				tmpframes[j] = get_frame(tmpframes[j]);

				int frame_len = tmpframes[0]->get_length();

				for(int k = 0; k < frame_len; k++)
				{
					if(peak[j] < fabs(tmpframes[j]->buffer[k]))
						peak[j] = fabs(tmpframes[j]->buffer[k]);
				}
			}
			current_pts = tmpframes[0]->get_end_pts();

			if(fragment_len < tmpframes[0]->get_buffer_length())
				break;
		}

		for(int j = 0; j < total_in_buffers; j++)
			release_aframe(tmpframes[j]);

// Normalize all tracks
		double max = 0;
		for(int i = 0; i < total_in_buffers; i++)
		{
			if(peak[i] > max)
				max = peak[i];
		}
		if(!separate_tracks)
		{
			for(int i = 0; i < total_in_buffers; i++)
			{
				peak[i] = max;
			}
		}

		for(int i = 0; i < total_in_buffers; i++)
			scale[i] = DB::fromdb(db_over) / peak[i];
		writing = 1;
	}

	if(writing)
	{
		int buflen = aframes[0]->get_length();

		for(int i = 0; i < total_in_buffers; i++)
		{
			for(int j = 0; j < buflen; j++)
				aframes[i]->buffer[j] *= scale[i];
		}

		if(end_pts - aframes[0]->get_end_pts() < EPSILON)
			result = 1;
	}
	return result;
}
