
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
#include "mainprogress.h"
#include "normalize.h"
#include "normalizewindow.h"
#include "picon_png.h"
#include "units.h"
#include "vframe.h"

#include <stdio.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

REGISTER_PLUGIN(NormalizeMain)






NormalizeMain::NormalizeMain(PluginServer *server)
 : PluginAClient(server)
{
	load_defaults();
}

NormalizeMain::~NormalizeMain()
{
	save_defaults();
	delete defaults;
}

char* NormalizeMain::plugin_title() { return N_("Normalize"); }
int NormalizeMain::is_realtime() { return 0; }
int NormalizeMain::is_multichannel() { return 1; }


VFrame* NormalizeMain::new_picon()
{
	return new VFrame(picon_png);
}


int NormalizeMain::load_defaults()
{
	char directory[BCTEXTLEN];

// set the default directory
	sprintf(directory, "%snormalize.rc", BCASTDIR);
	
// load the defaults

	defaults = new BC_Hash(directory);

	defaults->load();

	db_over = defaults->get("DBOVER", 0);
	separate_tracks = defaults->get("SEPERATE_TRACKS", 1);
	return 0;
}

int NormalizeMain::save_defaults()
{
	defaults->update("DBOVER", db_over);
	defaults->update("SEPERATE_TRACKS", separate_tracks);
	defaults->save();
	return 0;
}

int NormalizeMain::get_parameters()
{
	BC_DisplayInfo info;
	NormalizeWindow window(info.get_abs_cursor_x(), info.get_abs_cursor_y());
	window.create_objects(&db_over, &separate_tracks);
	int result = window.run_window();
	return result;
}

int NormalizeMain::start_loop()
{
	char string[BCTEXTLEN];
	sprintf(string, "%s...", plugin_title());
	progress = start_progress(string, (PluginClient::end - PluginClient::start) * 2);

	writing = 0;
	current_position = PluginClient::start;
	peak = new double[PluginClient::total_in_buffers];
	scale = new double[PluginClient::total_in_buffers];
	bzero(peak, sizeof(double) * PluginClient::total_in_buffers);
	return 0;
}


int NormalizeMain::process_loop(double **buffer, int64_t &write_length)
{
	int result = 0;
	int64_t fragment_len;

//printf("NormalizeMain::process_loop 1\n");
	if(writing)
	{
		fragment_len = PluginClient::in_buffer_size;
		if(current_position + fragment_len > PluginClient::end) fragment_len = PluginClient::end - current_position;
//printf("NormalizeMain::process_loop 2 %d %f\n", current_position, scale[0]);

		for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
			read_samples(buffer[i], i, current_position, fragment_len);
			for(int j = 0; j < fragment_len; j++)
				buffer[i][j] *= scale[i];
		}

//printf("NormalizeMain::process_loop 1 %d %f\n", current_position, scale[0]);
		current_position += fragment_len;
		write_length = fragment_len;
		result = progress->update(PluginClient::end - 
			PluginClient::start + 
			current_position - 
			PluginClient::start);
		if(current_position >= PluginClient::end) result = 1;
	}
	else
	{
// Get peak
//printf("NormalizeMain::process_loop 4\n");
		for(int i = PluginClient::start; 
			i < PluginClient::end && !result; 
			i += fragment_len)
		{
			fragment_len = PluginClient::in_buffer_size;
			if(i + fragment_len > PluginClient::end) fragment_len = PluginClient::end - i;
//printf("NormalizeMain::process_loop 5\n");

			for(int j = 0; j < PluginClient::total_in_buffers; j++)
			{
//printf("NormalizeMain::process_loop 6 %p\n", buffer);
				read_samples(buffer[j], j, i, fragment_len);
//printf("NormalizeMain::process_loop 7\n");
				
				for(int k = 0; k < fragment_len; k++)
				{
					if(peak[j] < fabs(buffer[j][k])) peak[j] = fabs(buffer[j][k]);
				}
			}
//printf("NormalizeMain::process_loop 8\n");
			result = progress->update(i - PluginClient::start);
//printf("NormalizeMain::process_loop 9\n");
		}

// Normalize all tracks
		double max = 0;
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
			if(peak[i] > max) max = peak[i];
		}
		if(!separate_tracks)
		{
			for(int i = 0; i < PluginClient::total_in_buffers; i++)
			{
				peak[i] = max;
			}
		}

		for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
			scale[i] = DB::fromdb(db_over) / peak[i];
		}
//printf("NormalizeMain::process_loop 10\n");

		char string[BCTEXTLEN];
		sprintf(string, "%s %.0f%%...", plugin_title(), (DB::fromdb(db_over) / max) * 100);
		progress->update_title(string);
// Start writing on next iteration
		writing = 1;
	}

	return result;
}

int NormalizeMain::stop_loop()
{
	progress->stop_progress();
	delete [] peak;
	delete [] scale;
	delete progress;
	return 0;
}
