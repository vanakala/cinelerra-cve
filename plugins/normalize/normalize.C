
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
#include "bchash.h"
#include "language.h"
#include "mainprogress.h"
#include "normalize.h"
#include "normalizewindow.h"
#include "picon_png.h"
#include "units.h"
#include "vframe.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN


NormalizeMain::NormalizeMain(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
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

void NormalizeMain::start_loop()
{
	char string[BCTEXTLEN];
	sprintf(string, "%s...", plugin_title());

	progress = start_progress(string, end_pts - start_pts);

	writing = 0;
	current_pts = start_pts;

	memset(peak, 0, sizeof(double) * MAXCHANNELS);
}


int NormalizeMain::process_loop(AFrame **aframes, int &write_length)
{
	int result = 0;
	int fragment_len;
	int duration_valid = 0;

	if(writing)
	{
		fragment_len = aframes[0]->buffer_length;
// Assume we always have samplerate here - we are reusing the same buffers
//  used below in analyzing part
		if(aframes[0]->samplerate)
		{
			if(current_pts + aframes[0]->to_duration(fragment_len) > end_pts)
				fragment_len = aframes[0]->to_samples(end_pts - current_pts);
		}
		for(int i = 0; i < total_in_buffers; i++)
		{
			aframes[i]->set_fill_request(current_pts, fragment_len);
			get_aframe(aframes[i]);

			for(int j = 0; j < aframes[0]->length; j++)
				aframes[i]->buffer[j] *= scale[i];
		}

		write_length = aframes[0]->length;
		current_pts = aframes[0]->pts + aframes[0]->duration;
		result = progress->update(end_pts - 2 * start_pts + current_pts);

		if(end_pts - current_pts < EPSILON)
			result = 1;
	}
	else
	{
// Get peak
		for(current_pts = start_pts; end_pts - current_pts > EPSILON;)
		{
			fragment_len = aframes[0]->buffer_length;
// Samplerate is not set at first
// We need samplerate of the asset here
			if(aframes[0]->samplerate)
			{
				if(current_pts + aframes[0]->to_duration(fragment_len) > end_pts)
					fragment_len = aframes[0]->to_samples(end_pts - current_pts);
				duration_valid = 1;
			}
			for(int j = 0; j < total_in_buffers; j++)
			{
				aframes[j]->set_fill_request(current_pts, fragment_len);
				get_aframe(aframes[j]);
				if(!duration_valid)
				{
					if(current_pts + aframes[j]->to_duration(fragment_len) > end_pts)
					{
					// truncate length afterwards - we have now samplerate
						fragment_len = aframes[j]->to_samples(end_pts - current_pts);
						aframes[j]->set_filled(fragment_len);
					}
					duration_valid = 1;
				}
				for(int k = 0; k < aframes[j]->length; k++)
				{
					if(peak[j] < fabs(aframes[j]->buffer[k])) 
						peak[j] = fabs(aframes[j]->buffer[k]);
				}
			}
			current_pts = aframes[0]->pts + aframes[0]->duration;
			if(progress->update(current_pts - start_pts))
				break;
		}

// Normalize all tracks
		double max = 0;
		for(int i = 0; i < total_in_buffers; i++)
		{
			if(peak[i] > max) max = peak[i];
		}
		if(!separate_tracks)
		{
			for(int i = 0; i < total_in_buffers; i++)
			{
				peak[i] = max;
			}
		}

		for(int i = 0; i < total_in_buffers; i++)
		{
			scale[i] = DB::fromdb(db_over) / peak[i];
		}

		char string[BCTEXTLEN];
		sprintf(string, "%s %.0f%%...", plugin_title(), (DB::fromdb(db_over) / max) * 100);
		progress->update_title(string);

// Start writing on next iteration
		current_pts = start_pts;
		writing = 1;
	}
	return result;
}

void NormalizeMain::stop_loop()
{
	progress->stop_progress();
	delete progress;
}
