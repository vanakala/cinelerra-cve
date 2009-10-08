
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
#include "picon_png.h"
#include "../../cinelerra/resample.h"
#include "resample.h"
#include "vframe.h"




PluginClient* new_plugin(PluginServer *server)
{
	return new ResampleEffect(server);
}



ResampleFraction::ResampleFraction(ResampleEffect *plugin, int x, int y)
 : BC_TextBox(x, y, 100, 1, (float)plugin->scale, 1, MEDIUMFONT, 6)
{
	this->plugin = plugin;
}

int ResampleFraction::handle_event()
{
	plugin->scale = atof(get_text());
	return 1;
}








ResampleWindow::ResampleWindow(ResampleEffect *plugin, int x, int y)
 : BC_Window(PROGRAM_NAME ": Resample", 
 				x - 160,
				y - 75,
 				320, 
				150, 
				320, 
				150,
				0,
				0,
				1)
{
	this->plugin = plugin;
}

void ResampleWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Scale factor:")));
	y += 20;
	add_subwindow(new ResampleFraction(plugin, x, y));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	flush();
}








ResampleEffect::ResampleEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	load_defaults();
}

ResampleEffect::~ResampleEffect()
{
	save_defaults();
	delete defaults;
}

char* ResampleEffect::plugin_title() { return N_("Resample"); }

VFrame* ResampleEffect::new_picon()
{
	return new VFrame(picon_png);
}

void ResampleEffect::reset()
{
	resample = 0;
}

int ResampleEffect::get_parameters()
{
	BC_DisplayInfo info;
	ResampleWindow window(this, info.get_abs_cursor_x(), info.get_abs_cursor_y());
	window.create_objects();
	int result = window.run_window();
	
	return result;
}


int ResampleEffect::load_defaults()
{
	char directory[BCTEXTLEN];

// set the default directory
	sprintf(directory, "%sresample.rc", BCASTDIR);
// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	scale = defaults->get("SCALE", (double)1);
	return 0;
}

int ResampleEffect::save_defaults()
{
	defaults->update("SCALE", scale);
	defaults->save();
	return 0;
}



int ResampleEffect::start_loop()
{
	if(PluginClient::interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, 
			(int64_t)((double)(PluginClient::end - PluginClient::start) / scale));
	}

	current_position = PluginClient::start;
	total_written = 0;

	resample = new Resample(0, 1);
	return 0;
}

int ResampleEffect::stop_loop()
{
	if(PluginClient::interactive)
	{
		progress->stop_progress();
		delete progress;
	}
	return 0;
}

int ResampleEffect::process_loop(double *buffer, int64_t &write_length)
{
	int result = 0;

// Length to read based on desired output size
	int64_t size = (int64_t)((double)PluginAClient::in_buffer_size * scale);
	int64_t predicted_total = (int64_t)((double)(PluginClient::end - PluginClient::start) / scale + 0.5);

	double *input = new double[size];

	read_samples(input, 0, current_position, size);
	current_position += size;

	resample->resample_chunk(input, 
		size, 
		1000000, 
		(int)(1000000.0 / scale), 
		0);


	if(resample->get_output_size(0))
	{
		int64_t output_size = resample->get_output_size(0);

		if(output_size)
		{
			total_written += output_size;
		}

// Trim output to predicted length of stretched selection.
		if(total_written > predicted_total)
		{
			output_size -= total_written - predicted_total;
			result = 1;
		}

		resample->read_output(buffer, 0, output_size);

		write_length = output_size;
	}

	if(PluginClient::interactive) result = progress->update(total_written);
//printf("TimeStretch::process_loop 1\n");

	delete [] input;
//printf("TimeStretch::process_loop 2\n");
	return result;
}












