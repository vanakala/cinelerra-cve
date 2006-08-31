#ifndef CHROMAKEY_H
#define CHROMAKEY_H




#include "colorpicker.h"
#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"


class ChromaKey;
class ChromaKey;
class ChromaKeyWindow;

enum {
	CHROMAKEY_POSTPROCESS_NONE,
	CHROMAKEY_POSTPROCESS_BLUR,
	CHROMAKEY_POSTPROCESS_DILATE
};

class ChromaKeyConfig
{
public:
	ChromaKeyConfig();

	void copy_from(ChromaKeyConfig &src);
	int equivalent(ChromaKeyConfig &src);
	void interpolate(ChromaKeyConfig &prev, 
		ChromaKeyConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	int get_color();

	// Output mode
	bool  show_mask;
	// Key color definition
	float red;
	float green;
	float blue;
	// Key shade definition
	float min_brightness;
	float max_brightness;
	float saturation;
	float min_saturation;
	float tolerance;
	// Mask feathering
	float in_slope;
	float out_slope;
	float alpha_offset;
	// Spill light compensation
	float spill_threshold;
	float spill_amount;
};

class ChromaKeyColor : public BC_GenericButton
{
public:
	ChromaKeyColor(ChromaKey *plugin, 
		ChromaKeyWindow *gui, 
		int x, 
		int y);

	int handle_event();

	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyMinBrightness : public BC_FSlider
{
	public:
		ChromaKeyMinBrightness(ChromaKey *plugin, int x, int y);
		int handle_event();
		ChromaKey *plugin;
};

class ChromaKeyMaxBrightness : public BC_FSlider
{
	public:
		ChromaKeyMaxBrightness(ChromaKey *plugin, int x, int y);
		int handle_event();
		ChromaKey *plugin;
};

class ChromaKeySaturation : public BC_FSlider
{
	public:
		ChromaKeySaturation(ChromaKey *plugin, int x, int y);
		int handle_event();
		ChromaKey *plugin;
};

class ChromaKeyMinSaturation : public BC_FSlider
{
	public:
		ChromaKeyMinSaturation(ChromaKey *plugin, int x, int y);
		int handle_event();
		ChromaKey *plugin;
};



class ChromaKeyTolerance : public BC_FSlider
{
public:
	ChromaKeyTolerance(ChromaKey *plugin, int x, int y);
	int handle_event();
	ChromaKey *plugin;
};

class ChromaKeyInSlope : public BC_FSlider
{
	public:
		ChromaKeyInSlope(ChromaKey *plugin, int x, int y);
		int handle_event();
		ChromaKey *plugin;
};

class ChromaKeyOutSlope : public BC_FSlider
{
	public:
		ChromaKeyOutSlope(ChromaKey *plugin, int x, int y);
		int handle_event();
		ChromaKey *plugin;
};

class ChromaKeyAlphaOffset : public BC_FSlider
{
	public:
		ChromaKeyAlphaOffset(ChromaKey *plugin, int x, int y);
		int handle_event();
		ChromaKey *plugin;
};

class ChromaKeySpillThreshold : public BC_FSlider
{
public:
	ChromaKeySpillThreshold(ChromaKey *plugin, int x, int y);
	int handle_event();
	ChromaKey *plugin;
};
class ChromaKeySpillAmount : public BC_FSlider
{
public:
	ChromaKeySpillAmount(ChromaKey *plugin, int x, int y);
	int handle_event();
	ChromaKey *plugin;
};

class ChromaKeyUseColorPicker : public BC_GenericButton
{
public:
	ChromaKeyUseColorPicker(ChromaKey *plugin, ChromaKeyWindow *gui, int x, int y);
	int handle_event();
	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyColorThread : public ColorThread
{
public:
	ChromaKeyColorThread(ChromaKey *plugin, ChromaKeyWindow *gui);
	int handle_new_color(int output, int alpha);
	ChromaKey *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeyShowMask : public BC_CheckBox
{
public:
	ChromaKeyShowMask(ChromaKey *plugin, int x, int y);
	int handle_event();
	ChromaKey *plugin;
};



class ChromaKeyWindow : public BC_Window
{
public:
	ChromaKeyWindow(ChromaKey *plugin, int x, int y);
	~ChromaKeyWindow();

	void create_objects();
	int close_event();
	void update_sample();

	ChromaKeyColor *color;
	ChromaKeyUseColorPicker *use_colorpicker;
	ChromaKeyMinBrightness *min_brightness;
	ChromaKeyMaxBrightness *max_brightness;
	ChromaKeySaturation *saturation;
	ChromaKeyMinSaturation *min_saturation;
	ChromaKeyTolerance *tolerance;
	ChromaKeyInSlope *in_slope;
	ChromaKeyOutSlope *out_slope;
	ChromaKeyAlphaOffset *alpha_offset;
	ChromaKeySpillThreshold *spill_threshold;
	ChromaKeySpillAmount *spill_amount;
	ChromaKeyShowMask *show_mask;
	BC_SubWindow *sample;
	ChromaKey *plugin;
	ChromaKeyColorThread *color_thread;
};



PLUGIN_THREAD_HEADER(ChromaKey, ChromaKeyThread, ChromaKeyWindow)


class ChromaKeyServer : public LoadServer
{
public:
	ChromaKeyServer(ChromaKey *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ChromaKey *plugin;
};

class ChromaKeyPackage : public LoadPackage
{
public:
	ChromaKeyPackage();
	int y1, y2;
};

class ChromaKeyUnit : public LoadClient
{
public:
	ChromaKeyUnit(ChromaKey *plugin, ChromaKeyServer *server);
	void process_package(LoadPackage *package);
	template <typename component_type> void process_chromakey(int components, component_type max, bool use_yuv, ChromaKeyPackage *pkg);
	bool is_same_color(float r, float g, float b, float rk,float bk,float gk, float color_threshold, float light_threshold, int key_main_component);

	ChromaKey *plugin;

};




class ChromaKey : public PluginVClient
{
public:
	ChromaKey(PluginServer *server);
	~ChromaKey();
	
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_configuration();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int show_gui();
	int set_string();
	void raise_window();
	void update_gui();

	ChromaKeyConfig config;
	VFrame *input, *output;
	ChromaKeyServer *engine;
	ChromaKeyThread *thread;
	BC_Hash *defaults;
};








#endif







