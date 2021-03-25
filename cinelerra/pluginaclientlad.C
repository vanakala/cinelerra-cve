
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
#include "clip.h"
#include "data/lad_picon_png.h"
#include "bchash.h"
#include "bctitle.h"
#include "filexml.h"
#include "language.h"
#include "pluginaclientlad.h"
#include "pluginwindow.h"

#ifdef HAVE_LADSPA
#include <ctype.h>
#include <string.h>


PluginAClientConfig::PluginAClientConfig()
{
	reset();
}

PluginAClientConfig::~PluginAClientConfig()
{
	delete_objects();
}

void PluginAClientConfig::reset()
{
	total_ports = 0;
	port_data = 0;
	port_type = 0;
}

void PluginAClientConfig::delete_objects()
{
	if(port_data)
		delete [] port_data;
	if(port_type)
		delete [] port_type;
	reset();
}


int PluginAClientConfig::equivalent(PluginAClientConfig &that)
{
	if(that.total_ports != total_ports) return 0;
	for(int i = 0; i < total_ports; i++)
		if(!EQUIV(port_data[i], that.port_data[i])) return 0;
	return 1;
}

// Need PluginServer to do this.
void PluginAClientConfig::copy_from(PluginAClientConfig &that)
{
	if(total_ports != that.total_ports)
	{
		delete_objects();
		total_ports = that.total_ports;
		port_data = new LADSPA_Data[total_ports];
		port_type = new int[total_ports];
	}

	for(int i = 0; i < total_ports; i++)
	{
		port_type[i] = that.port_type[i];
		port_data[i] = that.port_data[i];
	}

}

void PluginAClientConfig::interpolate(PluginAClientConfig &prev, 
	PluginAClientConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	copy_from(prev);
}

void PluginAClientConfig::initialize(PluginServer *server)
{
	delete_objects();

	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
			LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
		{
			total_ports++;
		}
	}

	port_data = new LADSPA_Data[total_ports];
	port_type = new int[total_ports];

	int current_port = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
			LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
		{
// Convert LAD default to default value
			float value = 0.0;
			const LADSPA_PortRangeHint *lad_hint = &server->lad_descriptor->PortRangeHints[i];
			LADSPA_PortRangeHintDescriptor hint_desc = lad_hint->HintDescriptor;

// Store type of port for GUI use
			port_type[current_port] = PORT_NORMAL;
			if(LADSPA_IS_HINT_SAMPLE_RATE(hint_desc))/* &&
				!LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc) &&
				!LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc))*/
			{
// LAD frequency table
				port_type[current_port] = PORT_FREQ_INDEX;
			}
			else
			if(LADSPA_IS_HINT_TOGGLED(hint_desc))
				port_type[current_port] = PORT_TOGGLE;
			else
			if(LADSPA_IS_HINT_INTEGER(hint_desc))
				port_type[current_port] = PORT_INTEGER;

// Get default of port using crazy hinting system
			if(LADSPA_IS_HINT_DEFAULT_0(hint_desc))
				value = 0.0;
			else
			if(LADSPA_IS_HINT_DEFAULT_1(hint_desc))
				value = 1.0;
			else
			if(LADSPA_IS_HINT_DEFAULT_100(hint_desc))
				value = 100.0;
			else
			if(LADSPA_IS_HINT_DEFAULT_440(hint_desc))
			{
				if(port_type[current_port] == PORT_FREQ_INDEX)
					value = 440.0 / 44100;
				else
					value = 440.0;
			}
			else
			if(LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_desc))
				value = lad_hint->UpperBound;
			else
			if(LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_desc))
				value = lad_hint->LowerBound;
			else
			if(LADSPA_IS_HINT_DEFAULT_LOW(hint_desc))
			{
				if(LADSPA_IS_HINT_LOGARITHMIC(hint_desc))
					value = exp(log(lad_hint->LowerBound) * 0.25 +
						log(lad_hint->UpperBound) * 0.75);
				else
					value = lad_hint->LowerBound * 0.25 +
						lad_hint->UpperBound * 0.75;
			}
			else
			if(LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_desc))
			{
				if(LADSPA_IS_HINT_LOGARITHMIC(hint_desc))
					value = exp(log(lad_hint->LowerBound) * 0.5 +
						log(lad_hint->UpperBound) * 0.5);
				else
					value = lad_hint->LowerBound * 0.5 +
						lad_hint->UpperBound * 0.5;
			}
			else
			if(LADSPA_IS_HINT_DEFAULT_HIGH(hint_desc))
			{
				if(LADSPA_IS_HINT_LOGARITHMIC(hint_desc))
					value = exp(log(lad_hint->LowerBound) * 0.75 +
						log(lad_hint->UpperBound) * 0.25);
				else
					value = lad_hint->LowerBound * 0.75 +
						lad_hint->UpperBound * 0.25;
			}

			port_data[current_port] = value;
			current_port++;
		}
	}
}


PluginACLientToggle::PluginACLientToggle(PluginAClientLAD *plugin,
	int x,
	int y,
	LADSPA_Data *output)
 : BC_CheckBox(x, y, (int)(*output))
{
	this->plugin = plugin;
	this->output = output;
}

int PluginACLientToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


PluginACLientILinear::PluginACLientILinear(PluginAClientLAD *plugin,
	int x,
	int y,
	LADSPA_Data *output,
	int min,
	int max)
 : BC_IPot(x, y, (int)(*output), min, max)
{
	this->plugin = plugin;
	this->output = output;
}

int PluginACLientILinear::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


PluginACLientFLinear::PluginACLientFLinear(PluginAClientLAD *plugin,
	int x,
	int y,
	LADSPA_Data *output,
	double min,
	double max)
 : BC_FPot(x, y, *output, min, max)
{
	this->plugin = plugin;
	this->output = output;
	set_precision(0.01);
}

int PluginACLientFLinear::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


PluginACLientFreq::PluginACLientFreq(PluginAClientLAD *plugin,
	int x,
	int y,
	LADSPA_Data *output,
	int translate_linear)
 : BC_QPot(x, 
	y, 
	translate_linear ?
		(int)(*output * plugin->get_project_samplerate()) :
		(int)*output)
{
	this->plugin = plugin;
	this->output = output;
	this->translate_linear = translate_linear;
}

int PluginACLientFreq::handle_event()
{
	*output = translate_linear ?
		(float)get_value() / plugin->get_project_samplerate() :
		get_value();
	plugin->send_configure_change();
	return 1;
}


PluginAClientWindow::PluginAClientWindow(PluginAClientLAD *plugin, 
	int x, 
	int y)
 : PluginWindow(plugin,
	x,
	y,
	500, 
	plugin->config.total_ports * 30 + 60)
{
	PluginServer *server = plugin->server;
	char string[BCTEXTLEN];
	int current_port = 0;
	x = 10;
	y = 10;
	int x2 = 300;
	int x3 = 335;
	int title_vmargin = 5;
	int max_w = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
			LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
		{
			int w = get_text_width(MEDIUMFONT, (char*)server->lad_descriptor->PortNames[i]);
			if(w > max_w) max_w = w;
		}
	}

	x2 = max_w + 20;
	x3 = max_w + 55;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
			LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
		{
			const LADSPA_PortRangeHint *lad_hint = &server->lad_descriptor->PortRangeHints[i];
			LADSPA_PortRangeHintDescriptor hint_desc = lad_hint->HintDescriptor;
			int use_min = LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc);
			int use_max = LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc);
			sprintf(string, "%s:", server->lad_descriptor->PortNames[i]);

			switch(plugin->config.port_type[current_port])
			{
			case PluginAClientConfig::PORT_NORMAL:
				{
					PluginACLientFLinear *flinear;
					float min = use_min ? lad_hint->LowerBound : 0;
					float max = use_max ? lad_hint->UpperBound : 100;
					add_subwindow(new BC_Title(x, 
						y + title_vmargin, 
						string));
					add_subwindow(flinear = new PluginACLientFLinear(
						plugin,
						(current_port % 2) ? x2 : x3,
						y,
						&plugin->config.port_data[current_port],
						min,
						max));
					fpots.append(flinear);
					break;
				}
			case PluginAClientConfig::PORT_FREQ_INDEX:
				{
					PluginACLientFreq *freq;
					add_subwindow(new BC_Title(x, 
						y + title_vmargin, 
						string));
					add_subwindow(freq = new PluginACLientFreq(
						plugin,
						(current_port % 2) ? x2 : x3,
						y,
						&plugin->config.port_data[current_port], 0));
					freqs.append(freq);
					break;
				}
			case PluginAClientConfig::PORT_TOGGLE:
				{
					PluginACLientToggle *toggle;
					add_subwindow(new BC_Title(x, 
						y + title_vmargin, 
						string));
					add_subwindow(toggle = new PluginACLientToggle(
						plugin,
						(current_port % 2) ? x2 : x3,
						y,
						&plugin->config.port_data[current_port]));
					toggles.append(toggle);
					break;
				}
			case PluginAClientConfig::PORT_INTEGER:
				{
					PluginACLientILinear *ilinear;
					float min = use_min ? lad_hint->LowerBound : 0;
					float max = use_max ? lad_hint->UpperBound : 100;
					add_subwindow(new BC_Title(x, 
						y + title_vmargin, 
						string));
					add_subwindow(ilinear = new PluginACLientILinear(
						plugin,
						(current_port % 2) ? x2 : x3,
						y,
						&plugin->config.port_data[current_port],
						(int)min,
						(int)max));
					ipots.append(ilinear);
					break;
				}
			}
			current_port++;
			y += 30;
		}
	}

	y += 10;
	sprintf(string, _("Author: %s"), server->lad_descriptor->Maker);
	add_subwindow(new BC_Title(x, y, string));
	y += 20;
	sprintf(string, _("License: %s"), server->lad_descriptor->Copyright);
	add_subwindow(new BC_Title(x, y, string));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

PluginAClientWindow::~PluginAClientWindow()
{
}

void PluginAClientWindow::update()
{
}

PLUGIN_THREAD_METHODS


PluginAClientLAD::PluginAClientLAD(PluginServer *server)
 : PluginAClient(server)
{
	in_buffers = 0;
	total_inbuffers = 0;
	out_buffers = 0;
	total_outbuffers = 0;
	buffer_allocation = 0;
	lad_instance = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

PluginAClientLAD::~PluginAClientLAD()
{
	delete_buffers();
	delete_plugin();
	PLUGIN_DESTRUCTOR_MACRO
}

int PluginAClientLAD::is_multichannel()
{
	if(get_inchannels() > 1 || get_outchannels() > 1) return 1;
	return 0;
}

int PluginAClientLAD::get_inchannels()
{
	int result = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i]) &&
			LADSPA_IS_PORT_AUDIO(server->lad_descriptor->PortDescriptors[i]))
			result++;
	}
	return result;
}

int PluginAClientLAD::get_outchannels()
{
	int result = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_OUTPUT(server->lad_descriptor->PortDescriptors[i]) &&
			LADSPA_IS_PORT_AUDIO(server->lad_descriptor->PortDescriptors[i]))
			result++;
	}
	return result;
}

PLUGIN_CLASS_METHODS

char* PluginAClientLAD::lad_to_string(char *string, const char *input)
{
	strcpy(string, input);
	for(int j = 0; j < strlen(string); j++)
	{
		if(string[j] == ' ' ||
			string[j] == '<' ||
			string[j] == '>') string[j] = '_';
	}
	return string;
}

char* PluginAClientLAD::lad_to_upper(char *string, const char *input)
{
	lad_to_string(string, input);
	for(int j = 0; j < strlen(string); j++)
		string[j] = toupper(string[j]);
	return string;
}


void PluginAClientLAD::load_defaults()
{
	char string[BCTEXTLEN];

	strcpy(string, plugin_title());
	for(int i = 0; i < strlen(string); i++)
		if(string[i] == ' ') string[i] = '_';
	strcat(string, ".rc");

// load the defaults
	defaults = load_defaults_file(string);

	config.initialize(server);

	int current_port = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
			LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
		{
			PluginAClientLAD::lad_to_upper(string, 
				(char*)server->lad_descriptor->PortNames[i]);

			config.port_data[current_port] = 
				defaults->get(string, 
					config.port_data[current_port]);
			current_port++;
		}
	}
}

void PluginAClientLAD::save_defaults()
{
	char string[BCTEXTLEN];
	int current_port = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
			LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
		{
// Convert LAD port name to default title
			PluginAClientLAD::lad_to_upper(string, 
				(char*)server->lad_descriptor->PortNames[i]);

			defaults->update(string, config.port_data[current_port]);
			current_port++;
		}
	}
	defaults->save();
}

void PluginAClientLAD::save_data(KeyFrame *keyframe)
{
	FileXML output;
	char string[BCTEXTLEN];

	output.tag.set_title(lad_to_upper(string, plugin_title()));

	int current_port = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
			LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
		{
// Convert LAD port name to default title
			PluginAClientLAD::lad_to_upper(string, 
				(char*)server->lad_descriptor->PortNames[i]);

			output.tag.set_property(string, config.port_data[current_port]);
			current_port++;
		}
	}

	output.append_tag();
	keyframe->set_data(output.string);
}

void PluginAClientLAD::read_data(KeyFrame *keyframe)
{
	FileXML input;
	char string[BCTEXTLEN];

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());
	config.initialize(server);

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is(lad_to_upper(string, plugin_title())))
			{
				int current_port = 0;
				for(int i = 0; i < server->lad_descriptor->PortCount; i++)
				{
					if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
						LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
					{
						PluginAClientLAD::lad_to_upper(string, 
							(char*)server->lad_descriptor->PortNames[i]);

						config.port_data[current_port] = 
							input.tag.get_property(string, 
								config.port_data[current_port]);
						current_port++;
					}
				}
			}
		}
	}
}

void PluginAClientLAD::delete_buffers()
{
	if(in_buffers)
	{
		for(int i = 0; i < total_inbuffers; i++) delete [] in_buffers[i];
	}
	if(out_buffers)
	{
		for(int i = 0; i < total_outbuffers; i++) delete [] out_buffers[i];
	}
	in_buffers = 0;
	out_buffers = 0;
	total_inbuffers = 0;
	total_outbuffers = 0;
	buffer_allocation = 0;
}

void PluginAClientLAD::delete_plugin()
{
	if(lad_instance)
	{
		if(server->lad_descriptor->deactivate)
			server->lad_descriptor->deactivate(lad_instance);
		server->lad_descriptor->cleanup(lad_instance);
	}
	lad_instance = 0;
}

void PluginAClientLAD::init_plugin(int total_in, int total_out, int size)
{
	if(buffer_allocation && buffer_allocation < size)
	{
		delete_buffers();
	}

	buffer_allocation = MAX(size, buffer_allocation);
	if(!in_buffers)
	{
		total_inbuffers = total_in;
		in_buffers = new LADSPA_Data*[total_inbuffers];
		for(int i = 0; i < total_inbuffers; i++) 
			in_buffers[i] = new LADSPA_Data[buffer_allocation];
	}

	if(!out_buffers)
	{
		total_outbuffers = total_out;
		out_buffers = new LADSPA_Data*[total_outbuffers];
		for(int i = 0; i < total_outbuffers; i++) 
			out_buffers[i] = new LADSPA_Data[buffer_allocation];
	}

	int need_reconfigure = 0;
	if(!lad_instance)
	{
		need_reconfigure = 1;
	}
	need_reconfigure |= load_configuration();

	if(need_reconfigure)
	{
		delete_plugin();
		lad_instance = server->lad_descriptor->instantiate(
			server->lad_descriptor,
			get_project_samplerate());

		int current_port = 0;
		for(int i = 0; i < server->lad_descriptor->PortCount; i++)
		{
			if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i]) &&
				LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i]))
			{
				server->lad_descriptor->connect_port(lad_instance,
					i,
					config.port_data + current_port);
				current_port++;
			}
			else
			if(LADSPA_IS_PORT_OUTPUT(server->lad_descriptor->PortDescriptors[i]) &&
				LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i]))
			{
				server->lad_descriptor->connect_port(lad_instance,
					i,
					&dummy_control_output);
			}
		}
		if(server->lad_descriptor->activate)
			server->lad_descriptor->activate(lad_instance);
		need_reconfigure = 0;
	}

	int current_port = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i]) &&
			LADSPA_IS_PORT_AUDIO(server->lad_descriptor->PortDescriptors[i]))
		{
			server->lad_descriptor->connect_port(lad_instance,
						i,
						in_buffers[current_port]);
			current_port++;
		}
	}

	current_port = 0;
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_OUTPUT(server->lad_descriptor->PortDescriptors[i]) &&
			LADSPA_IS_PORT_AUDIO(server->lad_descriptor->PortDescriptors[i]))
		{
			server->lad_descriptor->connect_port(lad_instance,
						i,
						out_buffers[current_port]);
			current_port++;
		}
	}
}

void PluginAClientLAD::process_realtime(AFrame *input, AFrame *output)
{
	int size = input->get_length();
	int in_channels = get_inchannels();
	int out_channels = get_outchannels();

	init_plugin(in_channels, out_channels, size);

	for(int i = 0; i < in_channels; i++)
	{
		LADSPA_Data *in_buffer = in_buffers[i];
		for(int j = 0; j < size; j++)
			in_buffer[j] = input->buffer[j];
	}
	for(int i = 0; i < out_channels; i++)
		memset(out_buffers[i], 0, sizeof(float) * size);

	server->lad_descriptor->run(lad_instance, size);

	LADSPA_Data *out_buffer = out_buffers[0];

	if(input != output)
		output->copy_of(input);

	for(int i = 0; i < size; i++)
	{
		output->buffer[i] = out_buffer[i];
	}
}

void PluginAClientLAD::process_realtime(AFrame **input_frames,
	AFrame **output_frames)
{
	int size = input_frames[0]->get_length();
	int in_channels = get_inchannels();
	int out_channels = get_outchannels();

	init_plugin(in_channels, out_channels, size);

	for(int i = 0; i < in_channels; i++)
	{
		float *in_buffer = in_buffers[i];
		double *in_ptr;
		if(i < PluginClient::total_in_buffers)
			in_ptr = input_frames[i]->buffer;
		else
			in_ptr = input_frames[0]->buffer;
		for(int j = 0; j < size; j++)
			in_buffer[j] = in_ptr[j];
	}
	for(int i = 0; i < out_channels; i++)
		memset(out_buffers[i], 0, sizeof(float) * size);

	server->lad_descriptor->run(lad_instance, size);

	for(int i = 0; i < PluginClient::total_in_buffers; i++)
	{
		if(i < total_outbuffers)
		{
			LADSPA_Data *out_buffer = out_buffers[i];
			double *out_ptr = output_frames[i]->buffer;

			if(output_frames[i] != input_frames[i])
				output_frames[i]->copy_of(input_frames[i]);

			for(int j = 0; j < size; j++)
				out_ptr[j] = out_buffer[j];
		}
	}
}
#endif /* HAVE_LADSPA */
