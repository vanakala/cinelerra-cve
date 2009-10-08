
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

#include <string.h>









class InterpolateAllEffect : public PluginAClient
{
public:
	InterpolateAllEffect(PluginServer *server);
	~InterpolateAllEffect();

	char* plugin_title();
	int is_realtime();
	int is_multichannel();
	int get_parameters();
	int start_loop();
	int process_loop(double *buffer, long &output_lenght);
	int stop_loop();




	int state;
	enum
	{
		READING,
		WRITING
	};
	double sample1;
	double sample2;
	int current_position;
	double slope;
	double intercept;

	MainProgressBar *progress;
};




REGISTER_PLUGIN(InterpolateAllEffect)








InterpolateAllEffect::InterpolateAllEffect(PluginServer *server)
 : PluginAClient(server)
{
}

InterpolateAllEffect::~InterpolateAllEffect()
{
}




char* InterpolateAllEffect::plugin_title() { return N_("Interpolate"); }
int InterpolateAllEffect::is_realtime() { return 0; }
int InterpolateAllEffect::is_multichannel() { return 0; }


int InterpolateAllEffect::get_parameters()
{
	return 0;
}

int InterpolateAllEffect::start_loop()
{
	state = READING;
	char string[BCTEXTLEN];
	sprintf(string, "%s...", plugin_title());
	progress = start_progress(string, (PluginClient::end - PluginClient::start));
	current_position = PluginClient::start;
	return 0;
}

int InterpolateAllEffect::stop_loop()
{
	progress->stop_progress();
	delete progress;
	return 0;
}

int InterpolateAllEffect::process_loop(double *buffer, long &write_length)
{
//printf("InterpolateAllEffect::process_loop 1\n");
	int result = 0;
	if(state == READING)
	{
// Read a certain amount before the first sample
		int leadin = PluginClient::in_buffer_size;
//printf("InterpolateAllEffect::process_loop 2\n");
		double buffer[leadin];
		if(PluginClient::start - leadin < 0) leadin = PluginClient::start;
		read_samples(buffer, PluginClient::start - leadin, leadin);
		sample1 = buffer[leadin - 1];

// Read a certain amount before the last sample
		leadin = PluginClient::in_buffer_size;
		if(PluginClient::end - leadin < 0) leadin = PluginClient::end;
		read_samples(buffer, PluginClient::end - leadin, leadin);
		sample2 = buffer[leadin - 1];
		state = WRITING;
		current_position = PluginClient::start;

// Get slope and intercept
		slope = (sample2 - sample1) /
			(PluginClient::end - PluginClient::start);
		intercept = sample1;
//printf("InterpolateAllEffect::process_loop 3\n");
	}
//printf("InterpolateAllEffect::process_loop 4\n");

	int fragment_len = PluginClient::in_buffer_size;
	if(current_position + fragment_len > PluginClient::end) fragment_len = PluginClient::end - current_position;
	double intercept2 = intercept + slope * (current_position - PluginClient::start);
	for(int i = 0; i < fragment_len; i++)
	{
		buffer[i] = intercept2 + slope * i;
	}
	current_position += fragment_len;
	write_length = fragment_len;
	result = progress->update(PluginClient::end - 
		PluginClient::start + 
		current_position - 
		PluginClient::start);
	if(current_position >= PluginClient::end) result = 1;
//printf("InterpolateAllEffect::process_loop 5\n");


	return result;
}



