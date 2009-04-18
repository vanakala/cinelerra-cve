#ifndef DELAYAUDIO_H
#define DELAYAUDIO_H

#include "bchash.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"
#include <vector>



class DelayAudio;
class DelayAudioWindow;
class DelayAudioTextBox;




class DelayAudioConfig
{
public:
	DelayAudioConfig();
	
	int equivalent(DelayAudioConfig &that);
	void copy_from(DelayAudioConfig &that);
	double length;
};



class DelayAudioThread : public Thread
{
public:
	DelayAudioThread(DelayAudio *plugin);
	~DelayAudioThread();
	
	void run();
	
	Mutex completion;
	DelayAudioWindow *window;
	DelayAudio *plugin;
};



class DelayAudioWindow : public BC_Window
{
public:
	DelayAudioWindow(DelayAudio *plugin, int x, int y);
	~DelayAudioWindow();
	
	int create_objects();
	int close_event();
	void update_gui();

	DelayAudio *plugin;
	DelayAudioTextBox *length;
};




class DelayAudioTextBox : public BC_TextBox
{
public:
	DelayAudioTextBox(DelayAudio *plugin, int x, int y);
	~DelayAudioTextBox();
	
	int handle_event();
	
	DelayAudio *plugin;
};




class DelayAudio : public PluginAClient
{
public:
	DelayAudio(PluginServer *server);
	~DelayAudio();

	VFrame* new_picon();
	char* plugin_title();
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(int64_t size, double *input_ptr, double *output_ptr);
	int show_gui();
	void raise_window();
	int set_string();





	void load_configuration();
	void update_gui();



	std::vector<double> buffer;
	DelayAudioThread *thread;
	BC_Hash *defaults;
	DelayAudioConfig config;
};




#endif
