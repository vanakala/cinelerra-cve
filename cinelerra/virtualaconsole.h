#ifndef ARENDERTHREAD_H
#define ARENDERTHREAD_H


#include "arender.inc"
#include "filethread.inc"     // RING_BUFFERS
#include "virtualconsole.h"

class VirtualAConsole : public VirtualConsole
{
public:
	VirtualAConsole(RenderEngine *renderengine, ARender *arender);
	virtual ~VirtualAConsole();

	void new_input_buffer(int ring_buffer);
	void delete_input_buffer(int ring_buffer);
	int set_transport(int reverse, float speed);
	int total_ring_buffers();
	void get_playable_tracks();
	void run();

	int process_buffer(long input_len,
		long input_position,
		int last_buffer,
		long absolute_position);
		void process_console();

// set up virtual console and buffers
	int init_rendering(int duplicate);
//	int build_virtual_console(int duplicate, long current_position);
	VirtualNode* new_toplevel_node(Track *track, Module *module, int track_number);

// delete buffers, tables, and mutexes
	int stop_rendering(int duplicate);

// process a buffer
	int process_buffer(int buffer, long input_len, long input_position, long absolute_position);
	int send_last_output_buffer();  // cause audio device to quit

// pointers to audio to read from disk
// (float*)(Track*)[Ring buffer]
	double **buffer_in[RING_BUFFERS];
	ARender *arender;
};


#endif
