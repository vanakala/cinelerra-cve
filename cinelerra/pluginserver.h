#ifndef PLUGINSERVER_H
#define PLUGINSERVER_H

// inherited by plugins


#include "arraylist.h"
#include "attachmentpoint.inc"
#include "edl.inc"
#include "floatauto.inc"
#include "floatautos.inc"
#include "keyframe.inc"
#include "ladspa.h"
#include "mainprogress.inc"
#include "maxbuffers.h"
#include "menueffects.inc"
#include "module.inc"
#include "mutex.h"
#include "mwindow.inc"
#include "plugin.inc"
#include "pluginaclientlad.inc"
#include "pluginclient.inc"
#include "pluginserver.inc"
#include "theme.inc"
#include "thread.h"
#include "track.inc"
#include "vframe.inc"

#include <stdio.h>
#include <string.h>
#include <unistd.h>





class PluginServer
{
public:
	PluginServer();
	PluginServer(char *path);
	PluginServer(PluginServer &);
	virtual ~PluginServer();


	friend class PluginAClientLAD;
	friend class PluginAClientConfig;
	friend class PluginAClientWindow;

// open a plugin and wait for commands
// Get information for plugindb if master.
#define PLUGINSERVER_IS_LAD 2
#define PLUGINSERVER_NOT_RECOGNIZED 1
#define PLUGINSERVER_OK 0
	int open_plugin(int master, 
		EDL *edl, 
		Plugin *plugin,
		int lad_index = -1);
// close the plugin
	int close_plugin();    
	void dump();


// queries
	void set_title(char *string);
// Generate title for display
	void generate_display_title(char *string);
// Get keyframes for configuration
	KeyFrame* get_prev_keyframe(long position);
	KeyFrame* get_next_keyframe(long position);
	long get_source_start();
// Get interpolation used by EDL
	int get_interpolation_type();
// Get or create keyframe for writing, depending on editing status
	KeyFrame* get_keyframe();
// Create new theme object
	Theme* new_theme();




// =============================== for realtime plugins
// save configuration of plugin
	void save_data(KeyFrame *keyframe);          
// Update EDL and playback engines to reflect changes
	void sync_parameters();
// set for realtime processor usage
	int set_realtime_sched();
	int get_gui_status();
// Raise the GUI
	void raise_window();
// cause the plugin to show the GUI
	void show_gui();          
// Update GUI with keyframe settings
	void update_gui();
	void update_title();
	void client_side_close();
	

	int set_string(char *string);      // set the string that appears on the plugin title
// give the buffers and sizes and prepare processing realtime data
	int init_realtime(int realtime_sched,
		int total_in_buffers,
		int buffer_size);   
// process the data in the buffers
	void process_realtime(VFrame **input, 
			VFrame **output, 
			long current_position,
			long total_len);  // Total len for transitions
	void process_realtime(double **input, 
			double **output,
			long current_position, 
			long fragment_size,
			long total_len);
// Called by rendering client to cause the GUI to display something with the data.
	void send_render_gui(void *data);
	void send_render_gui(void *data, int size);
// Called by MWindow to cause GUI to display
	void render_gui(void *data);
	void render_gui(void *data, int size);

// Send the boundary autos of the next fragment
	int set_automation(FloatAutos *autos, FloatAuto **start_auto, FloatAuto **end_auto, int reverse);



// set the fragment position of a buffer before rendering
	int arm_buffer(int buffer_number, 
				long in_fragment_position, 
				long out_fragment_position,
				int double_buffer_in,
				int double_buffer_out);
// Detach all the shared buffers.
	int detach_buffers();

	int send_buffer_info();







// ============================ for non realtime plugins
// start processing data in plugin
	int start_loop(long start, long end, long buffer_size, int total_buffers);
// Do one iteration of a nonrealtime plugin and return if finished
	int process_loop(VFrame **buffers, long &write_length);
	int process_loop(double **buffers, long &write_length);
	int stop_loop();
	int read_frame(VFrame *buffer, int channel, long start_position);
	int read_samples(double *buffer, int channel, long start_position, long total_samples);
	int read_samples(double *buffer, long start_position, long total_samples);

// For non realtime, prompt user for parameters, waits for plugin to finish and returns a result
	int get_parameters();
	int get_samplerate();      // get samplerate produced by plugin
	double get_framerate();     // get framerate produced by plugin
	int get_project_samplerate();            // get samplerate of project data before processing
	double get_project_framerate();         // get framerate of project data before processing
	int set_path(char *path);    // required first
// Used by PluginArray and MenuEffects to get user parameters and progress bar.
// Set pointer to mwindow for opening GUI and reconfiguring EDL.
	void set_mwindow(MWindow *mwindow);
// Used in VirtualConsole
// Set pointer to AttachmentPoint to render GUI.
	void set_attachmentpoint(AttachmentPoint *attachmentpoint);
// Set pointer to a default keyframe when there is no plugin
	void set_keyframe(KeyFrame *keyframe);
// Set pointer to menueffect window
	void set_prompt(MenuEffectPrompt *prompt);
	int set_interactive();             // make this the master plugin for progress bars
// add track to the list of affected tracks for a non realtime plugin
	int set_module(Module *module);
	int set_error();         // flag to send plugin an error on next request
	MainProgressBar* start_progress(char *string, long length);

	int long get_written_samples();   // after samples are written, get the number written
	int long get_written_frames();   // after frames are written, get the number written


// buffers
	long out_buffer_size;   // size of a send buffer to the plugin
	long in_buffer_size;    // size of a recieve buffer from the plugin
	int total_in_buffers;
	int total_out_buffers;

// number of double buffers for each channel
	ArrayList<int> ring_buffers_in;    
	ArrayList<int> ring_buffers_out;
// Parameters for automation.  Setting autos to 0 disables automation.
	FloatAuto **start_auto, **end_auto;
	FloatAutos *autos;
	int reverse;

// size of each buffer
	ArrayList<long> realtime_in_size;
	ArrayList<long> realtime_out_size;

// When arming buffers need to know the offsets in all the buffers and which
// double buffers for each channel before rendering.
	ArrayList<long> offset_in_render;
	ArrayList<long> offset_out_render;
	ArrayList<long> double_buffer_in_render;
	ArrayList<long> double_buffer_out_render;

// don't delete buffers if they belong to a virtual module
	int shared_buffers;
// Send new buffer information for next render
	int new_buffers;


	int plugin_open;                 // Whether or not the plugin is open.
// Specifies what type of plugin.
	int realtime, multichannel, fileio;  
// Plugin generates media
	int synthesis;
// What data types the plugin can handle.  One of these is set.
	int audio, video, theme;
// Can display a GUI
	int uses_gui;
// Plugin is a transition
	int transition;
// name of plugin in english.  Compared against the title value in the plugin.
	char *title;               
	long written_samples, written_frames;
	char *path;           // location of plugin on disk
	char *data_text;      // pointer to the data that was requested by a save_data command
	char *args[4];
	int total_args;
	int error_flag;      // send plugin an error code on next request
	ArrayList<Module*> *modules;     // tracks affected by this plugin during a non realtime operation
	AttachmentPoint *attachmentpoint;
	MWindow *mwindow;
// Pointer to keyframe when plugin is not available
	KeyFrame *keyframe;
	AttachmentPoint *attachment;
// Storage of keyframes and GUI status
	Plugin *plugin;
// Storage of session parameters
	EDL *edl;
	MenuEffectPrompt *prompt;
	int gui_on;

	VFrame *temp_frame;

// Icon for Asset Window
	VFrame *picon;

private:
	int reset_parameters();
	int cleanup_plugin();

// Base class created by client
	PluginClient *client;
// Handle from dlopen.  Plugins are opened once at startup and stored in the master
// plugindb.
	void *plugin_fd;
// Pointers to C functions
	PluginClient* (*new_plugin)(PluginServer*);

// LAD support
	int is_lad;
	LADSPA_Descriptor_Function lad_descriptor_function;
	const LADSPA_Descriptor *lad_descriptor;
};


#endif
