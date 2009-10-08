
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

#include "clip.h"
#include "data/lad_picon_png.h"
#include "bchash.h"
#include "filexml.h"
#include "pluginaclientlad.h"
#include "pluginserver.h"
#include "vframe.h"

#include <ctype.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



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
//printf("PluginAClientConfig::copy_from 1 %f %f\n", port_data[i], that.port_data[i]);
	}

}

void PluginAClientConfig::interpolate(PluginAClientConfig &prev, 
	PluginAClientConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
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
	float min,
	float max)
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
		(int)(*output * plugin->PluginAClient::project_sample_rate) :
		(int)*output)
{
//printf("PluginACLientFreq::PluginACLientFreq 1 %f\n", *output);
	this->plugin = plugin;
	this->output = output;
	this->translate_linear = translate_linear;
}

int PluginACLientFreq::handle_event()
{
	*output = translate_linear ?
		(float)get_value() / plugin->PluginAClient::project_sample_rate :
		get_value();
//printf("PluginACLientFreq::handle_event 1 %f %d %d\n", *output, get_value(), plugin->PluginAClient::project_sample_rate);
	plugin->send_configure_change();
	return 1;
}









PluginAClientWindow::PluginAClientWindow(PluginAClientLAD *plugin, 
	int x, 
	int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	500, 
	plugin->config.total_ports * 30 + 60, 
	500, 
	plugin->config.total_ports * 30 + 60, 
	0, 
	1)
{
	this->plugin = plugin;
}

PluginAClientWindow::~PluginAClientWindow()
{
}


int PluginAClientWindow::create_objects()
{
	PluginServer *server = plugin->server;
	char string[BCTEXTLEN];
	int current_port = 0;
	int x = 10;
	int y = 10;
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

// printf("PluginAClientWindow::create_objects 1 %s type=%d lower: %d %f upper: %d %f\n", 
// string,
// plugin->config.port_type[current_port],
// use_min,
// lad_hint->LowerBound, 
// use_max,
// lad_hint->UpperBound);

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
						&plugin->config.port_data[current_port],
0
/*						(plugin->config.port_type[current_port] == 
							PluginAClientConfig::PORT_FREQ_INDEX
*/));
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
//printf("PluginAClientWindow::create_objects 2\n");
		}
	}

	y += 10;
	sprintf(string, _("Author: %s"), server->lad_descriptor->Maker);
	add_subwindow(new BC_Title(x, y, string));
	y += 20;
	sprintf(string, _("License: %s"), server->lad_descriptor->Copyright);
	add_subwindow(new BC_Title(x, y, string));
}

int PluginAClientWindow::close_event()
{
	set_done(1);
	return 1;
}












PLUGIN_THREAD_OBJECT(PluginAClientLAD, PluginAClientThread, PluginAClientWindow)





PluginAClientLAD::PluginAClientLAD(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	in_buffers = 0;
	total_inbuffers = 0;
	out_buffers = 0;
	total_outbuffers = 0;
	buffer_allocation = 0;
	lad_instance = 0;
}

PluginAClientLAD::~PluginAClientLAD()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete_buffers();
	delete_plugin();
}

int PluginAClientLAD::is_realtime()
{
	return 1;
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


char* PluginAClientLAD::plugin_title()
{
	return (char*)server->lad_descriptor->Name;
}

int PluginAClientLAD::uses_gui()
{
	return 1;
}

int PluginAClientLAD::is_synthesis()
{
	return 1;
}

VFrame* PluginAClientLAD::new_picon()
{
	return new VFrame(lad_picon_png);
}

SHOW_GUI_MACRO(PluginAClientLAD, PluginAClientThread)
RAISE_WINDOW_MACRO(PluginAClientLAD)
SET_STRING_MACRO(PluginAClientLAD)
LOAD_CONFIGURATION_MACRO(PluginAClientLAD, PluginAClientConfig)

void PluginAClientLAD::update_gui()
{
}

char* PluginAClientLAD::lad_to_string(char *string, char *input)
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

char* PluginAClientLAD::lad_to_upper(char *string, char *input)
{
	lad_to_string(string, input);
	for(int j = 0; j < strlen(string); j++)
		string[j] = toupper(string[j]);
	return string;
}


int PluginAClientLAD::load_defaults()
{
	char directory[BCTEXTLEN];
	char string[BCTEXTLEN];


	strcpy(string, plugin_title());
	for(int i = 0; i < strlen(string); i++)
		if(string[i] == ' ') string[i] = '_';
// set the default directory
	sprintf(directory, "%s%s.rc", BCASTDIR, string);
//printf("PluginAClientLAD::load_defaults %s\n", directory);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();
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
//printf("PluginAClientLAD::load_defaults %d %f\n", current_port, config.port_data[current_port]);
			current_port++;
		}
	}
	return 0;
}


int PluginAClientLAD::save_defaults()
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
//printf("PluginAClientLAD::save_defaults %d %f\n", current_port, config.port_data[current_port]);
			current_port++;
		}
	}
	defaults->save();
	return 0;
}



void PluginAClientLAD::save_data(KeyFrame *keyframe)
{
	FileXML output;
	char string[BCTEXTLEN];

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title(lad_to_upper(string, plugin_title()));

	int current_port = 0;
//printf("PluginAClientLAD::save_data %d\n", server->lad_descriptor->PortCount);
	for(int i = 0; i < server->lad_descriptor->PortCount; i++)
	{
		if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i] &&
			LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i])))
		{
// Convert LAD port name to default title
			PluginAClientLAD::lad_to_upper(string, 
				(char*)server->lad_descriptor->PortNames[i]);

			output.tag.set_property(string, config.port_data[current_port]);
//printf("PluginAClientLAD::save_data %d %f\n", current_port, config.port_data[current_port]);
			current_port++;
		}
	}

	output.append_tag();
	output.terminate_string();
}

void PluginAClientLAD::read_data(KeyFrame *keyframe)
{
	FileXML input;
	char string[BCTEXTLEN];

	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	config.initialize(server);

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
//printf("PluginAClientLAD::read_data %s\n", input.tag.get_title());
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
//printf("PluginAClientLAD::read_data %d %f\n", current_port, config.port_data[current_port]);
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
//printf("PluginAClientLAD::init_plugin 1\n");
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
			PluginAClient::project_sample_rate);

		int current_port = 0;
		for(int i = 0; i < server->lad_descriptor->PortCount; i++)
		{
			if(LADSPA_IS_PORT_INPUT(server->lad_descriptor->PortDescriptors[i]) &&
				LADSPA_IS_PORT_CONTROL(server->lad_descriptor->PortDescriptors[i]))
			{
				server->lad_descriptor->connect_port(lad_instance,
					i,
					config.port_data + current_port);
//printf("PluginAClientLAD::init_plugin %d %f\n", current_port, config.port_data[current_port]);
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
//printf("PluginAClientLAD::init_plugin 10\n");
}

int PluginAClientLAD::process_realtime(int64_t size, 
	double *input_ptr, 
	double *output_ptr)
{
	int in_channels = get_inchannels();
	int out_channels = get_outchannels();
	init_plugin(in_channels, out_channels, size);

//printf("PluginAClientLAD::process_realtime 1 %p\n", lad_instance);
	for(int i = 0; i < in_channels; i++)
	{
		LADSPA_Data *in_buffer = in_buffers[i];
		for(int j = 0; j < size; j++)
			in_buffer[j] = input_ptr[j];
	}
	for(int i = 0; i < out_channels; i++)
		bzero(out_buffers[i], sizeof(float) * size);
//printf("PluginAClientLAD::process_realtime 4\n");

	server->lad_descriptor->run(lad_instance, size);
//printf("PluginAClientLAD::process_realtime 5\n");

	LADSPA_Data *out_buffer = out_buffers[0];
	for(int i = 0; i < size; i++)
	{
		output_ptr[i] = out_buffer[i];
	}
//printf("PluginAClientLAD::process_realtime 6\n");
	return size;
}

int PluginAClientLAD::process_realtime(int64_t size, 
	double **input_ptr, 
	double **output_ptr)
{
	int in_channels = get_inchannels();
	int out_channels = get_outchannels();
// printf("PluginAClientLAD::process_realtime 2 %p %d %d %d %d\n", 
// lad_instance, in_channels, out_channels, PluginClient::total_in_buffers, PluginClient::total_out_buffers);
	init_plugin(in_channels, out_channels, size);
//printf("PluginAClientLAD::process_realtime 2 %p\n", lad_instance);

	for(int i = 0; i < in_channels; i++)
	{
		float *in_buffer = in_buffers[i];
		double *in_ptr;
		if(i < PluginClient::total_in_buffers)
			in_ptr = input_ptr[i];
		else
			in_ptr = input_ptr[0];
		for(int j = 0; j < size; j++)
			in_buffer[j] = in_ptr[j];
	}
//printf("PluginAClientLAD::process_realtime 2 %p\n", lad_instance);
	for(int i = 0; i < out_channels; i++)
		bzero(out_buffers[i], sizeof(float) * size);
//printf("PluginAClientLAD::process_realtime 2 %p\n", lad_instance);

	server->lad_descriptor->run(lad_instance, size);
//printf("PluginAClientLAD::process_realtime 2 %p\n", lad_instance);

	for(int i = 0; i < PluginClient::total_out_buffers; i++)
	{
		if(i < total_outbuffers)
		{
			LADSPA_Data *out_buffer = out_buffers[i];
			double *out_ptr = output_ptr[i];
			for(int j = 0; j < size; j++)
				out_ptr[j] = out_buffer[j];
		}
	}
//printf("PluginAClientLAD::process_realtime 3 %p\n", lad_instance);
	return size;
}


 
