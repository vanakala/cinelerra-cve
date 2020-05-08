
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

#include "aframe.h"
#include "bchash.h"
#include "bctitle.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "../../cinelerra/resample.h"
#include "resample.h"

REGISTER_PLUGIN


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
 : PluginWindow(plugin->gui_string,
				x - 160,
				y - 75,
				320, 
				150)
{
	x = y = 10;
	add_subwindow(new BC_Title(x, y, _("Scale factor:")));
	y += 20;
	add_subwindow(new ResampleFraction(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}


ResampleEffect::ResampleEffect(PluginServer *server)
 : PluginAClient(server)
{
	resample = 0;
	input_frame = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ResampleEffect::~ResampleEffect()
{
	if(input_frame)
		delete input_frame;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void ResampleEffect::load_defaults()
{
// load the defaults
	defaults = load_defaults_file("resample.rc");

	scale = defaults->get("SCALE", (double)1);
}

void ResampleEffect::save_defaults()
{
	defaults->update("SCALE", scale);
	defaults->save();
}

int ResampleEffect::process_loop(AFrame *aframe)
{
	int result = 0;
	int output_size;

// Length to read based on desired output size
	int size = (int)(aframe->get_buffer_length() * scale);

	if(!resample)
		resample = new Resample(0, 1);

	if(!input_frame)
		input_frame = new AFrame(size);

	input_frame->set_fill_request(current_pts, size);
	get_frame(input_frame);

	if(!predicted_total)
		predicted_total = input_frame->to_samples((end_pts - start_pts) / scale);
	current_pts = input_frame->get_end_pts();

	resample->resample_chunk(input_frame->buffer,
		size, 
		input_frame->get_samplerate(),
		(int)(input_frame->get_samplerate() / scale),
		0);

	if(output_size = resample->get_output_size(0))
	{
		total_written += output_size;

// Trim output to predicted length of stretched selection.
		if(total_written > predicted_total)
		{
			output_size -= total_written - predicted_total;
			result = 1;
		}
		aframe->set_samplerate(input_frame->get_samplerate());
		aframe->set_filled(output_size);
		aframe->set_pts(output_pts);

		resample->read_output(aframe->buffer, 0, output_size);

		output_pts = aframe->get_end_pts();
	}

	return result;
}
