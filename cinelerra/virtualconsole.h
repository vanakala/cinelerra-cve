#ifndef COMMONRENDERTHREAD_H
#define COMMONRENDERTHREAD_H

#include "arraylist.h"
#include "commonrender.inc"
#include "maxbuffers.h"
#include "module.inc"
#include "mutex.inc"
#include "playabletracks.inc"
#include "renderengine.inc"
#include "thread.h"
#include "track.inc"
#include "virtualnode.inc"

class VirtualConsole : public Thread
{
public:
	VirtualConsole(RenderEngine *renderengine, CommonRender *commonrender, int data_type);
	virtual ~VirtualConsole();

	virtual void create_objects();
	void start_playback();
	virtual int total_ring_buffers();
	virtual void get_playable_tracks() {};
	int allocate_input_buffers();
	virtual void new_input_buffer(int ring_buffer) { };
	virtual void delete_input_buffer(int ring_buffer) { };
	void dump();

// Create ptrs to input buffers
	virtual void create_input_ptrs() {};
// Build the nodes
	void build_virtual_console(int persistant_plugins);
	virtual VirtualNode* new_toplevel_node(Track *track, Module *module, int track_number) { return 0; };
	Module* module_of(Track *track);
	Module* module_number(int track_number);
// Test for reconfiguration.
// If reconfiguration is coming up, truncate length and reset last_playback.
	int test_reconfigure(int64_t position, 
		int64_t &length,
		int &last_playback);

	virtual void run();

	RenderEngine *renderengine;
	CommonRender *commonrender;
// Total playable tracks
	int total_tracks;          
// Top level node for each playable track
	VirtualNode **virtual_modules;
// Order to process nodes
	ArrayList<VirtualNode*> render_list;


	int data_type;
// Store result of total_ring_buffers for destructor
	int ring_buffers;
// exit conditions
	int interrupt;
	int done;













	virtual int init_rendering(int duplicate) {};
	int sort_virtual_console();
	int delete_virtual_console();
	int delete_input_buffers();
	int swap_thread_buffer();
	int swap_input_buffer();

// Set duplicate when this virtual console is to share the old resources.
	int start_rendering(int duplicate);
	virtual int stop_rendering(int duplicate) {};

// for synchronizing start and stop
	int wait_for_completion();
	int wait_for_startup();

	virtual int send_last_output_buffer() {};




// playable tracks
	int current_input_buffer;      // input buffer being read from disk
	PlayableTracks *playable_tracks;


	Mutex *input_lock[MAX_BUFFERS];     // lock before sending input buffers through console
	Mutex *output_lock[MAX_BUFFERS];	// lock before loading input buffers
	Mutex *startup_lock;                // Lock before returning from start

// information for each buffer
	int last_playback[MAX_BUFFERS];      // last buffer in playback range
	int last_reconfigure[MAX_BUFFERS];   // last buffer before deletion and reconfiguration
	int64_t input_len[MAX_BUFFERS];         // number of samples to render from this buffer
	int64_t input_position[MAX_BUFFERS];    // highest numbered sample of this buffer in project
										// or frame of this buffer in project
	int64_t absolute_position[MAX_BUFFERS];  // Absolute position at start of buffer for peaks.
	int current_vconsole_buffer;           // input buffer being rendered by vconsole
};



#endif
