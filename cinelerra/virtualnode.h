#ifndef VIRTUALNODE_H
#define VIRTUALNODE_H

#include "arraylist.h"
#include "auto.inc"
#include "autos.inc"
#include "floatauto.inc"
#include "floatautos.inc"
#include "mwindow.inc"
#include "maxbuffers.h"
#include "patch.h"
#include "plugin.inc"
#include "pluginserver.inc"
#include "track.inc"
#include "transition.inc"
#include "virtualconsole.inc"

// The virtual node makes up the virtual console.
// It can be either a virtual module or a virtual plugin.


class VirtualNode
{
public:
	VirtualNode(RenderEngine *renderengine, 
		VirtualConsole *vconsole, 
		Module *real_module, 
		Plugin *real_plugin, 
		Track *track, 
		VirtualNode *parent_module, 
		int input_is_master, 
		int output_is_master, 
		int in, 
		int out);

	virtual ~VirtualNode();
	void dump(int indent);

// derived node creates buffers here
	virtual void new_output_buffer() { };
	virtual void new_input_buffer() { };


// expand plugins
	int expand(int persistant_plugins, int64_t current_position);
// create buffers and convenience pointers
	int expand_buffers();
// create convenience pointers to shared memory depending on the data type
	virtual int create_buffer_ptrs() {};
// create a node for a module
	int attach_virtual_module(Plugin *plugin, 
		int plugin_number, 
		int duplicate, 
		int64_t current_position);
// create a node for a plugin
	int attach_virtual_plugin(Plugin *plugin, 
		int plugin_number, 
		int duplicate, 
		int64_t current_position);
	virtual VirtualNode* create_module(Plugin *real_plugin, 
							Module *real_module, 
							Track *track) { return 0; };
	virtual VirtualNode* create_plugin(Plugin *real_plugin) { return 0; };

	int render_as_plugin(int64_t source_len,
						int64_t source_position,
						int ring_buffer,
						int64_t fragment_position,
						int64_t fragment_len);

	int get_plugin_input(int &double_buffer_in, int64_t &fragment_position_in,
					int &double_buffer_out, int64_t &fragment_position_out,
					int double_buffer, int64_t fragment_position);

// Get the order to render modules and plugins attached to this.
// Return 1 if not completed after this pass.
	int sort(ArrayList<VirtualNode*>*render_list);

// virtual plugins this module owns
	ArrayList<VirtualNode*> vplugins;
// Attachment point in Module if this is a virtual plugin
	AttachmentPoint *attachment;

	VirtualConsole *vconsole;
// module which created this node.
	VirtualNode *parent_module;
// use these to determine if this node is a plugin or module
// Top level virtual node of module
	Module *real_module;
// When this node is a plugin.  Redirected to the shared plugin in expansion.
	Plugin *real_plugin;


	Track *track;
	RenderEngine *renderengine;

// for rendering need to know if the buffer is a master or copy
// These are set in expand()
	int input_is_master;
	int output_is_master;
	int ring_buffers;       // number of buffers for master buffers
	int64_t buffer_size;         // number of units in a master segment
	int64_t fragment_size;       // number of units in a node segment
	int plugin_type;          // type of plugin in case user changes it
	int render_count;         // times this plugin has been added to the render list
	int waiting_real_plugin;  //  real plugin tests this to see if virtual plugin is waiting on it when sorting
// for deleting need to know if buffers are owned by someone else
	int shared_input;
	int shared_output;
// where this node should get its input and output from
	int in;
	int out;
// module needs to know where the input data for the next process is
	int data_in_input;
// plugin needs to know what buffer number each fragment position corresponds to
	int plugin_buffer_number;

// Mute automation.
// Return whether the next samples are muted and store the duration
// of the next status in fragment_len
	void get_mute_fragment(int64_t input_position,
				int &mute_constant, 
				int64_t &fragment_len, 
				Autos *autos);

// convenience routines for fade automation
	void get_fade_automation(double &slope,
		double &intercept,
		int64_t input_position,
		int64_t &slope_len,
		Autos *autos);
	void get_pan_automation(double &slope,
		double &intercept,
		int64_t input_position,
		int64_t &slope_len,
		Autos *autos,
		int channel);

	int init_automation(int &automate, 
				double &constant, 
				int64_t input_position,
				int64_t buffer_len,
				Autos *autos,
				Auto **before, 
				Auto **after);

	int init_slope(Autos *autos, Auto **before, Auto **after);
	int get_slope(Autos *autos, int64_t buffer_len, int64_t buffer_position);
	int advance_slope(Autos *autos);

protected:

// ======================= plugin automation
	FloatAutos *plugin_autos;
	FloatAuto *plugin_auto_before, *plugin_auto_after;

// temporary variables for automation
	Auto *current_auto;
	double slope_value;
	double slope_start;
	double slope_end;
	double slope_position;
	double slope;
	double value;

	int reverse;  // Temporary set for each render
	int64_t input_start, input_end;
	int64_t buffer_position; // position in both input and output buffer

private:
	int sort_as_module(ArrayList<VirtualNode*>*render_list, int &result, int &total_result);
	int sort_as_plugin(ArrayList<VirtualNode*>*render_list, int &result, int &total_result);
	int expand_as_module(int duplicate, int64_t current_position);
	int expand_as_plugin(int duplicate);
};



#endif
