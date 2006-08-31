#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "transportque.h"

#include <string.h>

class ReframeRT;
class ReframeRTWindow;

class ReframeRTConfig
{
public:
	ReframeRTConfig();
	void boundaries();
	int equivalent(ReframeRTConfig &src);
	void copy_from(ReframeRTConfig &src);
	void interpolate(ReframeRTConfig &prev, 
		ReframeRTConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	double scale;
	int stretch;
};


class ReframeRTScale : public BC_TumbleTextBox
{
public:
	ReframeRTScale(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
};

class ReframeRTStretch : public BC_Radial
{
public:
	ReframeRTStretch(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};

class ReframeRTDownsample : public BC_Radial
{
public:
	ReframeRTDownsample(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};

class ReframeRTWindow : public BC_Window
{
public:
	ReframeRTWindow(ReframeRT *plugin, int x, int y);
	~ReframeRTWindow();
	void create_objects();
	int close_event();
	ReframeRT *plugin;
	ReframeRTScale *scale;
	ReframeRTStretch *stretch;
	ReframeRTDownsample *downsample;
};

PLUGIN_THREAD_HEADER(ReframeRT, ReframeRTThread, ReframeRTWindow)

class ReframeRT : public PluginVClient
{
public:
	ReframeRT(PluginServer *server);
	~ReframeRT();

	PLUGIN_CLASS_MEMBERS(ReframeRTConfig, ReframeRTThread)

	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
};







REGISTER_PLUGIN(ReframeRT);



ReframeRTConfig::ReframeRTConfig()
{
	scale = 1.0;
	stretch = 0;
}

int ReframeRTConfig::equivalent(ReframeRTConfig &src)
{
	return fabs(scale - src.scale) < 0.0001 &&
		stretch == src.stretch;
}

void ReframeRTConfig::copy_from(ReframeRTConfig &src)
{
	this->scale = src.scale;
	this->stretch = src.stretch;
}

void ReframeRTConfig::interpolate(ReframeRTConfig &prev, 
	ReframeRTConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
//	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
//	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

//	this->scale = prev.scale * prev_scale + next.scale * next_scale;
	this->scale = prev.scale;
	this->stretch = prev.stretch;
}

void ReframeRTConfig::boundaries()
{
	if(fabs(scale) < 0.0001) scale = 0.0001;
}








ReframeRTWindow::ReframeRTWindow(ReframeRT *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	210, 
	160, 
	200, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

ReframeRTWindow::~ReframeRTWindow()
{
}

void ReframeRTWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Scale by amount:")));
	y += 20;
	scale = new ReframeRTScale(plugin, 
		this,
		x, 
		y);
	scale->create_objects();
	scale->set_increment(0.1);
	y += 30;
	add_subwindow(stretch = new ReframeRTStretch(plugin, 
		this,
		x, 
		y));
	y += 30;
	add_subwindow(downsample = new ReframeRTDownsample(plugin, 
		this,
		x, 
		y));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(ReframeRTWindow)


PLUGIN_THREAD_OBJECT(ReframeRT, ReframeRTThread, ReframeRTWindow)






ReframeRTScale::ReframeRTScale(ReframeRT *plugin, 
	ReframeRTWindow *gui,
	int x, 
	int y)
 : BC_TumbleTextBox(gui,
 	(float)plugin->config.scale,
	(float)-1000,
	(float)1000,
 	x, 
	y, 
	100)
{
	this->plugin = plugin;
}

int ReframeRTScale::handle_event()
{
	plugin->config.scale = atof(get_text());
	plugin->config.boundaries();
	plugin->send_configure_change();
	return 1;
}

ReframeRTStretch::ReframeRTStretch(ReframeRT *plugin,
	ReframeRTWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, plugin->config.stretch, _("Stretch"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ReframeRTStretch::handle_event()
{
	plugin->config.stretch = get_value();
	gui->downsample->update(!get_value());
	plugin->send_configure_change();
	return 1;
}


ReframeRTDownsample::ReframeRTDownsample(ReframeRT *plugin,
	ReframeRTWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, !plugin->config.stretch, _("Downsample"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ReframeRTDownsample::handle_event()
{
	plugin->config.stretch = !get_value();
	gui->stretch->update(!get_value());
	plugin->send_configure_change();
	return 1;
}






ReframeRT::ReframeRT(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}


ReframeRT::~ReframeRT()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* ReframeRT::plugin_title() { return N_("ReframeRT"); }
int ReframeRT::is_realtime() { return 1; }
int ReframeRT::is_synthesis() { return 1; }

#include "picon_png.h"
NEW_PICON_MACRO(ReframeRT)

SHOW_GUI_MACRO(ReframeRT, ReframeRTThread)

RAISE_WINDOW_MACRO(ReframeRT)

SET_STRING_MACRO(ReframeRT)

LOAD_CONFIGURATION_MACRO(ReframeRT, ReframeRTConfig)

int ReframeRT::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int64_t input_frame = get_source_start();
	KeyFrame *tmp_keyframe, *next_keyframe = get_prev_keyframe(get_source_start());
	int64_t tmp_position, next_position;
	int64_t segment_len;
	double input_rate = frame_rate;
	int is_current_keyframe;

	// if there are no keyframes, the default keyframe is used, and its position is always 0;
	// if there are keyframes, the first keyframe can be after the effect start (and it controls settings before it)
	// so let's calculate using a fake keyframe with the same settings but position == effect start
	KeyFrame *fake_keyframe = new KeyFrame();
	fake_keyframe->copy_from(next_keyframe);
	fake_keyframe->position = local_to_edl(get_source_start());
	next_keyframe = fake_keyframe;

	// calculate input_frame accounting for all previous keyframes
	do
	{
		tmp_keyframe = next_keyframe;
		next_keyframe = get_next_keyframe(tmp_keyframe->position+1, 0);

		tmp_position = edl_to_local(tmp_keyframe->position);
		next_position = edl_to_local(next_keyframe->position);

		read_data(tmp_keyframe);

		is_current_keyframe =
			next_position > start_position // the next keyframe is after the current position
			|| next_keyframe->position == tmp_keyframe->position // there are no more keyframes
			|| !next_keyframe->position; // there are no keyframes at all

		if (is_current_keyframe)
			next_position = start_position;

		segment_len = next_position - tmp_position;

		input_frame += (int64_t)(segment_len * config.scale);
	} while (!is_current_keyframe);

	// Change rate
	if (!config.stretch)
		input_rate *= config.scale;

	read_frame(frame,
		0,
		input_frame,
		input_rate);

	delete fake_keyframe;

	return 0;
}





int ReframeRT::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sreframert.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.scale = defaults->get("SCALE", config.scale);
	config.stretch = defaults->get("STRETCH", config.stretch);
	return 0;
}

int ReframeRT::save_defaults()
{
	defaults->update("SCALE", config.scale);
	defaults->update("STRETCH", config.stretch);
	defaults->save();
	return 0;
}

void ReframeRT::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("REFRAMERT");
	output.tag.set_property("SCALE", config.scale);
	output.tag.set_property("STRETCH", config.stretch);
	output.append_tag();
	output.terminate_string();
}

void ReframeRT::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("REFRAMERT"))
		{
			config.scale = input.tag.get_property("SCALE", config.scale);
			config.stretch = input.tag.get_property("STRETCH", config.stretch);
		}
	}
}

void ReframeRT::update_gui()
{
	if(thread)
	{
		int changed = load_configuration();

		if(changed)
		{
			thread->window->lock_window("ReframeRT::update_gui");
			thread->window->scale->update((float)config.scale);
			thread->window->stretch->update(config.stretch);
			thread->window->downsample->update(!config.stretch);
			thread->window->unlock_window();
		}
	}
}





