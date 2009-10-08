
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

	VFrame* new_picon();
	char* plugin_title();

	int process_buffer(int64_t size, 
		double *buffer,
		int64_t start_position,
		int sample_rate);
	int is_realtime();

#define FRAGMENT_SIZE 4096
	double *start_fragment;
	double *end_fragment;
	double start_sample;
	double end_sample;
	int64_t range_start;
	int64_t range_end;
};




REGISTER_PLUGIN(InterpolateAudioEffect)







InterpolateAudioEffect::InterpolateAudioEffect(PluginServer *server)
 : PluginAClient(server)
{
	start_fragment = 0;
	end_fragment = 0;
}

InterpolateAudioEffect::~InterpolateAudioEffect()
{
	if(start_fragment) delete [] start_fragment;
	if(end_fragment) delete [] end_fragment;
}




char* InterpolateAudioEffect::plugin_title()
{
	return N_("Interpolate");
}


int InterpolateAudioEffect::is_realtime()
{
	return 1;
}


#include "picon_png.h"
NEW_PICON_MACRO(InterpolateAudioEffect)



int InterpolateAudioEffect::process_buffer(int64_t size, 
	double *buffer,
	int64_t start_position,
	int sample_rate)
{
	double slope;
	double intercept;

	if(!start_fragment) start_fragment = new double[FRAGMENT_SIZE];
	if(!end_fragment) end_fragment = new double[FRAGMENT_SIZE];

	if(get_direction() == PLAY_FORWARD)
	{
// On first sample of range.  Get boundary samples of effect.
		if(get_source_position() == get_source_start())
		{
// Need to read before desired sample to diffuse transients after audio
// seeks.
			range_start = get_source_start();
			range_end = get_source_start() + get_total_len();
//printf("InterpolateAudioEffect::process_buffer 1 %p\n", start_fragment);
			read_samples(start_fragment,
				0,
				sample_rate,
				range_start - FRAGMENT_SIZE,
				FRAGMENT_SIZE);
			start_sample = start_fragment[FRAGMENT_SIZE - 1];
			read_samples(end_fragment,
				0,
				sample_rate,
				range_end - FRAGMENT_SIZE,
				FRAGMENT_SIZE);
			end_sample = end_fragment[FRAGMENT_SIZE - 1];
		}


		for(int i = 0; i < size; i++)
		{
			double end_fraction = (double)(i + start_position - range_start) / 
				(range_end - range_start);
			double start_fraction = 1.0 - end_fraction;
			double out_sample = start_sample * start_fraction + 
				end_sample * end_fraction;
			buffer[i] = out_sample;
		}
	}
	else
	{
// On first sample of range.  Get boundary samples of effect.
		if(get_source_position() == get_source_start() + get_total_len())
		{
// Need to read before desired sample to diffuse transients after audio
// seeks.
			range_start = get_source_start() + get_total_len();
			range_end = get_source_start();
			read_samples(start_fragment,
				0,
				sample_rate,
				range_start,
				FRAGMENT_SIZE);
			start_sample = start_fragment[0];
			read_samples(end_fragment,
				0,
				sample_rate,
				range_end,
				FRAGMENT_SIZE);
			end_sample = end_fragment[0];
		}


		for(int i = 0; i < size; i++)
		{
			double start_fraction = (double)(start_position - i - range_end) / 
				(range_start - range_end);
			double end_fraction = 1.0 - start_fraction;
			double out_sample = start_sample * start_fraction + 
				end_sample * end_fraction;
			buffer[i] = out_sample;
		}
	}
	return 0;
}


