#ifndef VRENDERTHREAD_H
#define VRENDERTHREAD_H

#include "guicast.h"
#include "virtualconsole.h"
#include "maxbuffers.h"
#include "vframe.inc"
#include "vrender.inc"
#include "vtrack.inc"

class VirtualVConsole : public VirtualConsole
{
public:
	VirtualVConsole(RenderEngine *renderengine, VRender *vrender);
	virtual ~VirtualVConsole();

// Create ptrs to input buffers
	void new_input_buffer(int ring_buffer);
	void delete_input_buffer(int ring_buffer);
	int total_ring_buffers();
	void get_playable_tracks();
	VirtualNode* new_toplevel_node(Track *track, Module *module, int track_number);

	int init_rendering(int duplicate);


	int stop_rendering(int duplicate);

	int process_buffer(long input_position); // start of buffer in project if forward / end of buffer if reverse

	int send_last_output_buffer();

	long absolute_frame;        // absolute frame the buffer starts on
// Pointers to frames to read from disk.  Single ring buffer
// (VFrame*)(Track*)
	VFrame **buffer_in;
	VRender *vrender;
};











#endif
