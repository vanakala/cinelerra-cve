#include "attachmentpoint.h"
#include "auto.h"
#include "automation.h"
#include "autos.h"
#include "floatauto.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
#include "mwindow.h"
#include "module.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "virtualconsole.h"
#include "virtualnode.h"

VirtualNode::VirtualNode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin,
		Track *track, 
		VirtualNode *parent_module, 
		int input_is_master,
		int output_is_master,
		int in,
		int out)
{
	this->renderengine = renderengine;
	this->vconsole = vconsole;
	this->real_module = real_module;
	this->real_plugin = real_plugin;
	this->track = track;
	this->parent_module = parent_module;
	this->ring_buffers = vconsole->total_ring_buffers();
	this->input_is_master = input_is_master;
	this->output_is_master = output_is_master;
	this->in = in;
	this->out = out;
	shared_input = 1;
	shared_output = 1;
	render_count = 0;
	plugin_type = 0;
	waiting_real_plugin = 0;
	plugin_buffer_number = 0;
	plugin_autos = 0;
	plugin_auto_before = plugin_auto_after = 0;
	attachment = 0;
//printf("VirtualNode::VirtualNode 1\n");
}

VirtualNode::~VirtualNode()
{
	vplugins.remove_all_objects();
//printf("VirtualNode::VirtualNode 2\n");
}

#define PRINT_INDENT for(int j = 0; j < indent; j++) printf(" ");

void VirtualNode::dump(int indent)
{
	PRINT_INDENT
	printf("VirtualNode %s\n", track->title);
	if(real_module)
	{
		PRINT_INDENT
		printf(" Plugins\n");
		for(int i = 0; i < vplugins.total; i++)
		{
			vplugins.values[i]->dump(indent + 2);
		}
	}
	else
	if(real_plugin)
	{
		printf("    Plugin %s\n", real_plugin->title);
	}
}

int VirtualNode::expand(int persistant_plugins, long current_position)
{
	expand_buffers();

//printf("VirtualNode::expand 1 %p\n", this);
// module needs to know where the input data for the next process is
	data_in_input = 1;
	if(real_module)
	{
//printf("VirtualNode::expand 3\n");
		expand_as_module(persistant_plugins, current_position);
//printf("VirtualNode::expand 4\n");
	}
	else
	if(real_plugin)
	{
// attach to a real plugin for a plugin
// plugin always takes data from input to output
//printf("VirtualNode::expand 5\n");
		expand_as_plugin(persistant_plugins);
	}
//printf("VirtualNode::expand 6\n");

	return 0;
}

int VirtualNode::expand_buffers()
{
// temp needed for the output
	if(!out)
	{
		new_output_buffer();
		shared_output = 0;
		output_is_master = 0;
	}

// temp needed for the output
	if(!in)
	{
		new_input_buffer();
		shared_input = 0;
		input_is_master = 0;
	}

	return 0;
}

int VirtualNode::expand_as_module(int duplicate, long current_position)
{
	Transition *transition = 0;

// create the plugins for this module
	for(int i = 0; i < track->plugin_set.total; i++)
	{
//printf("VirtualNode::expand_as_module 1 %p\n", track);
		Plugin *plugin = track->get_current_plugin(current_position, 
			i, 
			renderengine->command->get_direction(),
			0);

//printf("VirtualNode::expand_as_module 2 %d %p\n", i, plugin);
		if(plugin)
		{
//printf("VirtualNode::expand_as_module 3 %p %d\n", plugin, plugin->on);
			if(plugin->on)
			{
				int plugin_type = plugin->plugin_type;
				if(plugin_type == PLUGIN_SHAREDMODULE)
				{
// plugin is a module
//printf("VirtualNode::expand_as_module 3\n");
					attach_virtual_module(plugin,
						i, 
						duplicate, 
						current_position);
//printf("VirtualNode::expand_as_module 4\n");
				}
				else
				if(plugin_type == PLUGIN_SHAREDPLUGIN ||
					plugin_type == PLUGIN_STANDALONE)
				{
// plugin is a plugin
//printf("VirtualNode::expand_as_module 5\n");
					attach_virtual_plugin(plugin, 
						i, 
						duplicate, 
						current_position);
//printf("VirtualNode::expand_as_module 6\n");
				}
			}
		}
//printf("VirtualNode::expand_as_module 7\n");
	}
	return 0;
}

int VirtualNode::expand_as_plugin(int duplicate)
{
	plugin_type = real_plugin->plugin_type;

	if(plugin_type == PLUGIN_SHAREDPLUGIN)
	{
// Attached to a shared plugin.
// Get the real real plugin it's attached to.

// Redirect the real_plugin to the shared plugin.
		int real_module_number = real_plugin->shared_location.module;
		int real_plugin_number = real_plugin->shared_location.plugin;
		Module *real_module = 0;


		real_module = vconsole->module_number(real_module_number);
// module references are absolute so may get the wrong data type track.
		if(real_module &&
			real_plugin_number < real_module->total_attachments)
		{
			attachment = real_module->attachments[real_plugin_number];
// Attachment is NULL if off
			if(attachment)
			{
				real_plugin = attachment->plugin;

// Real plugin not on then null it.
				if(!real_plugin->on) real_plugin = 0;
			}
			else
				real_plugin = 0;
		}
		else
			real_plugin = 0;
	}






	if(plugin_type == PLUGIN_STANDALONE)
	{
// Get plugin server
		Module *module = vconsole->module_of(track);
		attachment = module->attachment_of(real_plugin);
	}






// Add to real plugin's list of virtual plugins for configuration updates
// and plugin_server initializations.
// Input and output are taken care of when the parent module creates this plugin.
// Get number for passing to render.
// real_plugin may become NULL after shared plugin test.
	if(real_plugin && attachment)
	{

		if(attachment) 
			plugin_buffer_number = attachment->attach_virtual_plugin(this);
	}




	return 0;
}

int VirtualNode::attach_virtual_module(Plugin *plugin, 
	int plugin_number, 
	int duplicate, 
	long current_position)
{
	if(plugin->on)
	{
//printf("VirtualNode::attach_virtual_module 1 %p\n", plugin);
		int real_module_number = plugin->shared_location.module;
	//printf("VirtualNode::attach_virtual_module 1\n");
		Module *real_module = vconsole->module_number(real_module_number);
	//printf("VirtualNode::attach_virtual_module 1\n");
		Track *track = real_module->track;
	// working data is now in output
		if(plugin->out) data_in_input = 0;
	//printf("VirtualNode::attach_virtual_module 1\n");
		VirtualNode *virtual_module = create_module(plugin,
			real_module,
			track);
	//printf("VirtualNode::attach_virtual_module 1\n");

		vplugins.append(virtual_module);
	//printf("VirtualNode::attach_virtual_module 1\n");
		virtual_module->expand(duplicate, current_position);
//printf("VirtualNode::attach_virtual_module 2\n");
	}

	return 0;
}

int VirtualNode::attach_virtual_plugin(Plugin *plugin, 
	int plugin_number, 
	int duplicate, 
	long current_position)
{
	if(plugin->on)
	{
// working data is now in output
		if(plugin->out) data_in_input = 0;

		VirtualNode *virtual_plugin = create_plugin(plugin);
		vplugins.append(virtual_plugin);
		virtual_plugin->expand(duplicate, current_position);
	}
	return 0;
}

int VirtualNode::sort(ArrayList<VirtualNode*>*render_list)
{
	int result = 0, total_result = 0;

//sleep(1);
//printf("VirtualNode::sort %p %p\n", real_module, real_plugin);
	if(real_module)
	{
		sort_as_module(render_list, result, total_result);
	}
	else
	if(real_plugin)
	{
		sort_as_plugin(render_list, result, total_result);
	}

	if(!result && total_result) result = total_result;
// if a plugin that wasn't patched out couldn't be rendered, try again

	return result;
}

int VirtualNode::sort_as_module(ArrayList<VirtualNode*>*render_list, int &result, int &total_result)
{

// Render plugins first.
	for(int i = 0; i < vplugins.total && !result; i++)
	{
// stop when rendering can't continue without another higher level module
		result = vplugins.values[i]->sort(render_list);

		if(result && !vplugins.values[i]->out)
		{
			total_result = 1;
			result = 0;
// couldn't render the last plugin but it wasn't patched out so continue to next plugin
		}
	}
//printf("VirtualNode::sort_as_module 3\n");

// All plugins rendered.
// Render this module.
	if(render_count == 0 && !result)
	{
		render_list->append(this);
		render_count++;
		result = 0;
	}
//printf("VirtualNode::sort_as_module 4\n");
	return 0;
}

int VirtualNode::sort_as_plugin(ArrayList<VirtualNode*>*render_list,
	int &result, 
	int &total_result)
{
// Plugin server does not exist at this point.
// need to know if plugin requires all inputs to be armed before rendering
//printf("VirtualNode::sort_as_plugin 1\n");
	int multichannel = 0, singlechannel = 0;
//sleep(1);


// Referenced plugin is off
	if(!attachment) return 0;

//printf("VirtualNode::sort_as_plugin 2 %p\n", attachment);
	if(plugin_type == PLUGIN_STANDALONE || plugin_type == PLUGIN_SHAREDPLUGIN)
	{
		multichannel = attachment->multichannel_shared(1);
		singlechannel = attachment->singlechannel();
	}

//printf("VirtualNode::sort_as_plugin 3\n");
	if(plugin_type == PLUGIN_STANDALONE && !multichannel)
	{
// unshared single channel plugin
// render now
//printf("VirtualNode::sort_as_plugin 4\n");
		if(!render_count)
		{
			render_list->append(this);
			render_count++;
			result = 0;
		}
//printf("VirtualNode::sort_as_plugin 5\n");
	}
	else
	if(plugin_type == PLUGIN_SHAREDPLUGIN || multichannel)
	{

// Shared plugin
//printf("VirtualNode::sort_as_plugin 6\n");
		if(!render_count)
		{
			if(singlechannel)
			{
// shared single channel plugin
// render now
//printf("VirtualNode::sort_as_plugin 7\n");
				render_list->append(this);
//printf("VirtualNode::sort_as_plugin 8\n");
				render_count++;
				result = 0;
			}
			else
			{
// shared multichannel plugin
// all buffers must be armed before rendering at the same time
				if(!waiting_real_plugin)
				{
//printf("VirtualNode::sort_as_plugin 9 %p\n", attachment);
					waiting_real_plugin = 1;
					if(real_plugin)
						result = attachment->sort(this);
//printf("VirtualNode::sort_as_plugin 10\n");

					render_list->append(this);
//printf("VirtualNode::sort_as_plugin 11\n");
					render_count++;
				}
				else
				{
// Assume it was rendered later in the first pass
					result = 0;
				}
			}
		}
	}
//printf("VirtualNode::sort_as_plugin 12\n");
	return 0;
}

int VirtualNode::get_plugin_input(int &ring_buffer_in, long &fragment_position_in,
							int &ring_buffer_out, long &fragment_position_out,
							int ring_buffer, long fragment_position)
{
	if(input_is_master)
	{
		ring_buffer_in = ring_buffer;
		fragment_position_in = fragment_position;
	}
	else
	{
		ring_buffer_in = 0;
		fragment_position_in = 0;
	}

	if(output_is_master)
	{
		ring_buffer_out = ring_buffer;
		fragment_position_out = fragment_position;
	}
	else
	{
		ring_buffer_out = 0;
		fragment_position_out = 0;
	}
}

int VirtualNode::render_as_plugin(long source_len,
		long source_position,
		int ring_buffer,
		long fragment_position,
		long fragment_len)
{
// need numbers for actual buffers
	int direction = renderengine->command->get_direction();
	int ring_buffer_in, ring_buffer_out;
	long fragment_position_in, fragment_position_out;
	int multichannel = 0;
//printf("VirtualNode::render_as_plugin 1 %p\n", attachment);

// Abort if no plugin
	if(!attachment ||
		!real_plugin ||
		!real_plugin->on) return 0;

//printf("VirtualNode::render_as_plugin 2\n");
}



void VirtualNode::get_mute_fragment(long input_position,
				int &mute_constant, 
				long &fragment_len, 
				Autos *autos)
{
	long mute_len;
	int direction = renderengine->command->get_direction();

	IntAuto *prev_keyframe = 0;
	IntAuto *next_keyframe = 0;
	prev_keyframe = (IntAuto*)autos->get_prev_auto(input_position, direction, (Auto*)prev_keyframe);
	next_keyframe = (IntAuto*)autos->get_next_auto(input_position, direction, (Auto*)next_keyframe);

	if(direction == PLAY_FORWARD)
	{
// Two distinct keyframes within range
		if(next_keyframe->position > prev_keyframe->position)
		{
			mute_constant = prev_keyframe->value;

			if(next_keyframe->position < input_position + fragment_len)
				fragment_len = next_keyframe->position - input_position;
		}
		else
// One keyframe within range
		{
			mute_constant = prev_keyframe->value;
		}
	}
	else
	{
// Two distinct keyframes within range
		if(next_keyframe->position < prev_keyframe->position)
		{
			mute_constant = next_keyframe->value;

			if(next_keyframe->position > input_position - fragment_len)
				fragment_len = input_position - next_keyframe->position;
		}
		else
// One keyframe within range
		{
			mute_constant = next_keyframe->value;
		}
	}
}



void VirtualNode::get_fade_automation(double &slope,
	double &intercept,
	long input_position,
	long &slope_len,
	Autos *autos)
{
	int direction = renderengine->command->get_direction();
	((FloatAutos*)autos)->get_fade_automation(slope,
		intercept,
		input_position,
		slope_len,
		direction);
}

void VirtualNode::get_pan_automation(double &slope,
	double &intercept,
	long input_position,
	long &slope_len,
	Autos *autos,
	int channel)
{
	int direction = renderengine->command->get_direction();
	intercept = 0;
	slope = 0;

	PanAuto *prev_keyframe = 0;
	PanAuto *next_keyframe = 0;
	prev_keyframe = (PanAuto*)autos->get_prev_auto(input_position, direction, (Auto*)prev_keyframe);
	next_keyframe = (PanAuto*)autos->get_next_auto(input_position, direction, (Auto*)next_keyframe);
	
	if(direction == PLAY_FORWARD)
	{
// Two distinct automation points within range
		if(next_keyframe->position > prev_keyframe->position)
		{
			slope = ((double)next_keyframe->values[channel] - prev_keyframe->values[channel]) / 
				((double)next_keyframe->position - prev_keyframe->position);
			intercept = ((double)input_position - prev_keyframe->position) * slope + 
				prev_keyframe->values[channel];

			if(next_keyframe->position < input_position + slope_len)
				slope_len = next_keyframe->position - input_position;
		}
		else
// One automation point within range
		{
			slope = 0;
			intercept = prev_keyframe->values[channel];
		}
	}
	else
	{
// Two distinct automation points within range
		if(next_keyframe->position < prev_keyframe->position)
		{
			slope = ((double)next_keyframe->values[channel] - prev_keyframe->values[channel]) / 
				((double)next_keyframe->position - prev_keyframe->position);
			intercept = ((double)input_position - prev_keyframe->position) * slope + 
				prev_keyframe->values[channel];

			if(next_keyframe->position > input_position - slope_len)
				slope_len = input_position - next_keyframe->position;
		}
		else
// One automation point within range
		{
			slope = 0;
			intercept = next_keyframe->values[channel];
		}
	}
}


int VirtualNode::init_automation(int &automate, 
				double &constant, 
				long input_position,
				long buffer_len,
				Autos *autos,
				Auto **before, 
				Auto **after)
{
	

	return autos->init_automation(buffer_position,
				input_start, 
				input_end, 
				automate, 
				constant, 
				input_position,
				buffer_len,
				before, 
				after,
				reverse);
}

int VirtualNode::init_slope(Autos *autos, Auto **before, Auto **after)
{
	return autos->init_slope(&current_auto,
				slope_start, 
				slope_value,
				slope_position, 
				input_start, 
				input_end, 
				before, 
				after,
				reverse);
}

int VirtualNode::get_slope(Autos *autos, long buffer_len, long buffer_position)
{
	return autos->get_slope(&current_auto, 
				slope_start, 
				slope_end, 
				slope_value, 
				slope, 
				buffer_len, 
				buffer_position,
				reverse);
}

int VirtualNode::advance_slope(Autos *autos)
{
	return autos->advance_slope(&current_auto, 
				slope_start, 
				slope_value,
				slope_position, 
				reverse);
}




