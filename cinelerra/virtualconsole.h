#ifndef COMMONRENDERTHREAD_H
#define COMMONRENDERTHREAD_H

#include "arraylist.h"
#include "commonrender.inc"
#include "module.inc"
#include "playabletracks.inc"
#include "renderengine.inc"
#include "track.inc"
#include "virtualnode.inc"

// Virtual console runs synchronously for audio and video in
// pull mode.
class VirtualConsole
{
public:
	VirtualConsole(RenderEngine *renderengine, 
		CommonRender *commonrender, 
		int data_type);
	virtual ~VirtualConsole();

	virtual void create_objects();
	virtual void get_playable_tracks();
	int allocate_input_buffers();
	virtual void new_input_buffer(int ring_buffer) { };
	virtual void delete_input_buffer(int ring_buffer) { };
	void start_playback();

// Called during each process buffer operation to reset the status
// of the attachments to unprocessed.
	void VirtualConsole::reset_attachments();
	void dump();


// Build the nodes
	void build_virtual_console(int persistent_plugins);

// Create a new entry node in subclass of desired type.
// was new_toplevel_node
	virtual VirtualNode* new_entry_node(Track *track, 
		Module *module, 
		int track_number);
// Append exit node to table when expansion hits the end of a tree.
	void append_exit_node(VirtualNode *node);

	Module* module_of(Track *track);
	Module* module_number(int track_number);
// Test for reconfiguration.
// If reconfiguration is coming up, truncate length and reset last_playback.
	int test_reconfigure(int64_t position, 
		int64_t &length,
		int &last_playback);


	RenderEngine *renderengine;
	CommonRender *commonrender;


// Total entry nodes.  Corresponds to the total playable tracks.
// Was total_tracks
	int total_entry_nodes;          
// Entry node for each playable track
// Was toplevel_nodes
	VirtualNode **entry_nodes;

// Exit node for each playable track.  Rendering starts here and data is pulled
// up the tree.  Every virtual module is an exit node.
	ArrayList<VirtualNode*> exit_nodes;


// Order to process nodes
// Replaced by pull system
//	ArrayList<VirtualNode*> render_list;


	int data_type;
// Store result of total_ring_buffers for destructor
// Pull method can't use ring buffers for input.
//	int ring_buffers;
// exit conditions
	int interrupt;
	int done;
// Trace the rendering path of the tree
	int debug_tree;












	virtual int init_rendering(int duplicate) {};
// Replaced by pull system
//	int sort_virtual_console();
	int delete_virtual_console();

// Set duplicate when this virtual console is to share the old resources.
	virtual int stop_rendering(int duplicate) {};

	virtual int send_last_output_buffer() {};


	PlayableTracks *playable_tracks;
};



#endif
