#ifndef PLUGINCLIENT_H
#define PLUGINCLIENT_H

// Base class inherited by all the different types of plugins.

#define BCASTDIR "~/.bcast/"

class PluginClient;


#include "arraylist.h"
#include "condition.h"
#include "keyframe.h"
#include "mainprogress.inc"
#include "maxbuffers.h"
#include "plugincommands.h"
#include "pluginserver.inc"
#include "theme.inc"
#include "vframe.h"


extern "C"
{
extern PluginClient* new_plugin(PluginServer *server);
}

class PluginClientAuto
{
public:
	float position;
	float intercept;
	float slope;
};




// Convenience functions

#define REGISTER_PLUGIN(class_title) \
PluginClient* new_plugin(PluginServer *server) \
{ \
	return new class_title(server); \
}


#define WINDOW_CLOSE_EVENT(window_class) \
int window_class::close_event() \
{ \
/* Set result to 1 to indicate a client side close */ \
	set_done(1); \
	return 1; \
}


#define PLUGIN_THREAD_HEADER(plugin_class, thread_class, window_class) \
class thread_class : public Thread \
{ \
public: \
	thread_class(plugin_class *plugin); \
	~thread_class(); \
	void run(); \
	window_class *window; \
	plugin_class *plugin; \
};


#define PLUGIN_THREAD_OBJECT(plugin_class, thread_class, window_class) \
thread_class::thread_class(plugin_class *plugin) \
 : Thread(0, 0, 1) \
{ \
	this->plugin = plugin; \
} \
 \
thread_class::~thread_class() \
{ \
	delete window; \
} \
	 \
void thread_class::run() \
{ \
	BC_DisplayInfo info; \
	window = new window_class(plugin,  \
		info.get_abs_cursor_x() - 75,  \
		info.get_abs_cursor_y() - 65); \
	window->create_objects(); \
 \
/* Only set it here so tracking doesn't update it until everything is created. */ \
 	plugin->thread = this; \
	int result = window->run_window(); \
	if(result) plugin->client_side_close(); \
}




#define PLUGIN_CLASS_MEMBERS(config_name, thread_name) \
	int load_configuration(); \
	VFrame* new_picon(); \
	char* plugin_title(); \
	int show_gui(); \
	int set_string(); \
	void raise_window(); \
	Defaults *defaults; \
	config_name config; \
	thread_name *thread;

#define PLUGIN_CONSTRUCTOR_MACRO \
	thread = 0; \
	defaults = 0; \
	load_defaults(); \

#define PLUGIN_DESTRUCTOR_MACRO \
	if(thread) \
	{ \
		thread->window->set_done(0); \
	} \
 \
	if(defaults) save_defaults(); \
	if(defaults) delete defaults;




#define SHOW_GUI_MACRO(plugin_class, thread_class) \
int plugin_class::show_gui() \
{ \
	load_configuration(); \
	thread_class *new_thread = new thread_class(this); \
	new_thread->start(); \
	return 0; \
}

#define RAISE_WINDOW_MACRO(plugin_class) \
void plugin_class::raise_window() \
{ \
	if(thread) \
	{ \
		thread->window->lock_window(); \
		thread->window->raise_window(); \
		thread->window->flush(); \
		thread->window->unlock_window(); \
	} \
}

#define SET_STRING_MACRO(plugin_class) \
int plugin_class::set_string() \
{ \
	if(thread) \
	{ \
		thread->window->lock_window(); \
		thread->window->set_title(gui_string); \
		thread->window->unlock_window(); \
	} \
	return 0; \
}

#define NEW_PICON_MACRO(plugin_class) \
VFrame* plugin_class::new_picon() \
{ \
	return new VFrame(picon_png); \
}

#define LOAD_CONFIGURATION_MACRO(plugin_class, config_class) \
int plugin_class::load_configuration() \
{ \
	KeyFrame *prev_keyframe, *next_keyframe; \
	prev_keyframe = get_prev_keyframe(get_source_position()); \
	next_keyframe = get_next_keyframe(get_source_position()); \
 \
	config_class old_config, prev_config, next_config; \
	old_config.copy_from(config); \
	read_data(prev_keyframe); \
	prev_config.copy_from(config); \
	read_data(next_keyframe); \
	next_config.copy_from(config); \
	config.interpolate(prev_config,  \
		next_config,  \
		(next_keyframe->position == prev_keyframe->position) ? \
			get_source_position() : \
			prev_keyframe->position, \
		(next_keyframe->position == prev_keyframe->position) ? \
			get_source_position() + 1 : \
			next_keyframe->position, \
		get_source_position()); \
 \
	if(!config.equivalent(old_config)) \
		return 1; \
	else \
		return 0; \
}






class PluginClient
{
public:
	PluginClient(PluginServer *server);
	virtual ~PluginClient();

	
// Queries for the plugin server.
	virtual int is_realtime();
	virtual int is_audio();
	virtual int is_video();
	virtual int is_fileio();
	virtual int is_theme();
	virtual int uses_gui();
	virtual int is_multichannel();
	virtual int is_synthesis();
	virtual int is_transition();
	virtual char* plugin_title();   // return the title of the plugin
	virtual VFrame* new_picon();
	virtual Theme* new_theme();






// Non realtime signal processors define these.
	virtual int get_samplerate();        // give the samplerate of the output for a non realtime plugin
	virtual double get_framerate();         // give the framerate of the output for a non realtime plugin
	virtual int delete_nonrealtime_parameters();
	virtual int start_plugin();         // run a non realtime plugin
	virtual int get_parameters();     // get information from user before non realtime processing
	virtual int64_t get_in_buffers(int64_t recommended_size);  // return desired size for input buffers
	virtual int64_t get_out_buffers(int64_t recommended_size);     // return desired size for output buffers
	virtual int start_loop();
	virtual int process_loop();
	virtual int stop_loop();




// Realtime commands for signal processors.
// These must be defined by the plugin itself.
	virtual int set_string();             // Set the string identifying the plugin to modules and patches.
// cause the plugin to show the gui
	virtual int show_gui();               
// cause the plugin to hide the gui
	void client_side_close();
	void update_display_title();
// Raise the GUI
	virtual void raise_window() {};
	virtual void update_gui() {};
	virtual void save_data(KeyFrame *keyframe) {};    // write the plugin settings to text in text format
	virtual void read_data(KeyFrame *keyframe) {};    // read the plugin settings from the text
	int send_hide_gui();                                    // should be sent when the GUI recieves a close event from the user
	int send_configure_change();                            // when this plugin is adjusted, propogate parameters to virtual plugins

	int get_configure_change();                             // get propogated configuration change from a send_configure_change
	virtual void plugin_process_realtime(double **input, 
		double **output, 
		int64_t current_position, 
		int64_t fragment_size,
		int64_t total_len) {};
	virtual void plugin_process_realtime(VFrame **input, 
		VFrame **output, 
		int64_t current_position,
		int64_t total_len) {};
// Called by plugin server to update GUI with rendered data.
	virtual void plugin_render_gui(void *data) {};
	virtual void plugin_render_gui(void *data, int size) {};
	virtual int plugin_process_loop(VFrame **buffers, int64_t &write_length) { return 1; };
	virtual int plugin_process_loop(double **buffers, int64_t &write_length) { return 1; };
	virtual int init_realtime_parameters();     // get parameters depending on video or audio
	int get_gui_status();
	char* get_gui_string();

// Used by themes
// Used by plugins which need to know where they are.
	char* get_path();

// Return keyframe objects.  If position -1 use edl selection
	KeyFrame* get_prev_keyframe(int64_t position);
	KeyFrame* get_next_keyframe(int64_t position);

// The meaning of these changes depending on the plugin type.

// For transitions the source_len is the length of the transition and
// is calculated in *Module.

// For realtime plugins the source_len is 0 and is calculated in 
// *AttachmentPoint.    This may not be used at all.
	int64_t get_source_len();

// For realtime plugins gets the start of the plugin
	int64_t get_source_start();

// Length of source.  For effects it's the plugin length.  For transitions
// it's the transition length.
	int64_t get_total_len();

// For transitions the source_position is the playback position relative
// to the start of the transition.

// For plugins the source_position is relative to the start of the plugin.
	int64_t get_source_position();

// Get interpolation used by EDL
	int get_interpolation_type();

// If automation is used
	int automation_used();
// Get the automation value for the position in the current fragment
// The position is relative to the current fragment
	float get_automation_value(int64_t position);




// Operations for file handlers
	virtual int open_file() { return 0; };
	virtual int get_audio_parameters() { return 0; };
	virtual int get_video_parameters() { return 0; };
	virtual int check_header(char *path) { return 0; };
	virtual int open_file(char *path, int wr, int rd) { return 1; };
	virtual int close_file() { return 0; };





// All plugins define these.
	virtual int load_defaults();       // load default settings for the plugin
	virtual int save_defaults();      // save the current settings as defaults




// Non realtime operations for signal processors.
	int plugin_start_loop(int64_t start, int64_t end, int64_t buffer_size, int total_buffers);
	int plugin_stop_loop();
	int plugin_process_loop();
	MainProgressBar* start_progress(char *string, int64_t length);
// get samplerate of project data before processing
	int get_project_samplerate();
// get framerate of project data before processing
	double get_project_framerate();
	int get_project_smp();
	int get_aspect_ratio(float &aspect_w, float &aspect_h);
	int write_frames(int64_t total_frames);  // returns 1 for failure / tells the server that all output channel buffers are ready to go
	int write_samples(int64_t total_samples);  // returns 1 for failure / tells the server that all output channel buffers are ready to go
	int plugin_get_parameters();
	char* get_defaultdir();     // Directory defaults should be stored in
	void set_interactive();

// Realtime operations.
	int reset();
	virtual int plugin_command_derived(int plugin_command) {}; // Extension of plugin_run for derived plugins
	int plugin_get_range();
	int plugin_init_realtime(int realtime_priority, 
		int total_in_buffers,
		int buffer_size);



// create pointers to buffers of the plugin's type before realtime rendering
	virtual int delete_buffer_ptrs();




// communication convenience routines for the base class
	int stop_gui_client();     
	int save_data_client();
	int load_data_client();
	int set_string_client(char *string);                // set the string identifying the plugin
	int send_cancelled();        // non realtime plugin sends when cancelled

// ================================= Buffers ===============================

// number of double buffers for each channel
	ArrayList<int> double_buffers_in;    
	ArrayList<int> double_buffers_out;
// When arming buffers need to know the offsets in all the buffers and which
// double buffers for each channel before rendering.
	ArrayList<int64_t> offset_in_render;
	ArrayList<int64_t> offset_out_render;
	ArrayList<int64_t> double_buffer_in_render;
	ArrayList<int64_t> double_buffer_out_render;
// total size of each buffer depends on if it's a master or node
	ArrayList<int64_t> realtime_in_size;
	ArrayList<int64_t> realtime_out_size;

// ================================= Automation ===========================

	ArrayList<PluginClientAuto> automation;

// ================================== Messages ===========================
	char gui_string[1024];          // string identifying module and plugin
	int master_gui_on;              // Status of the master gui plugin
	int client_gui_on;              // Status of this client's gui

	int show_initially;             // set to show a realtime plugin initially
// range in project for processing
	int64_t start, end;                
	int interactive;                // for the progress bar plugin
	int success;
	int total_out_buffers;          // total send buffers allocated by the server
	int total_in_buffers;           // total recieve buffers allocated by the server
	int wr, rd;                     // File permissions for fileio plugins.

// These give the largest fragment the plugin is expected to handle.
// size of a send buffer to the server
	int64_t out_buffer_size;  
// size of a recieve buffer from the server
	int64_t in_buffer_size;   
// Local parameters of plugin
	int sample_rate;
	double frame_rate;
// Operating system scheduling
	int realtime_priority;

// For transient plugins.
	int64_t source_len;
	int64_t source_position;
	int64_t total_len;
// Total number of processors available - 1
	int smp;  
	PluginServer *server;

private:

// File handlers:
//	Asset *asset;     // Point to asset structure in shared memory
};


#endif
