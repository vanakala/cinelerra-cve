#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "theme.h"
#include "transportque.inc"
#include "vframe.h"

#include <string.h>
#include <stdint.h>



class InterpolateVideo;
class InterpolateVideoWindow;


class InterpolateVideoConfig
{
public:
	InterpolateVideoConfig();

	void copy_from(InterpolateVideoConfig *config);
	int equivalent(InterpolateVideoConfig *config);

// Frame rate of input
	double input_rate;
// If 1, use the keyframes as beginning and end frames and ignore input rate
	int use_keyframes;
};




class InterpolateVideoRate : public BC_TextBox
{
public:
	InterpolateVideoRate(InterpolateVideo *plugin, 
		InterpolateVideoWindow *gui, 
		int x, 
		int y);
	int handle_event();
	InterpolateVideo *plugin;
	InterpolateVideoWindow *gui;
};

class InterpolateVideoRateMenu : public BC_ListBox
{
public:
	InterpolateVideoRateMenu(InterpolateVideo *plugin, 
		InterpolateVideoWindow *gui, 
		int x, 
		int y);
	int handle_event();
	InterpolateVideo *plugin;
	InterpolateVideoWindow *gui;
};

class InterpolateVideoKeyframes : public BC_CheckBox
{
public:
	InterpolateVideoKeyframes(InterpolateVideo *plugin,
		int x, 
		int y);
	int handle_event();
	InterpolateVideo *plugin;
};

class InterpolateVideoWindow : public BC_Window
{
public:
	InterpolateVideoWindow(InterpolateVideo *plugin, int x, int y);
	~InterpolateVideoWindow();

	void create_objects();
	int close_event();

	ArrayList<BC_ListBoxItem*> frame_rates;
	InterpolateVideo *plugin;

	InterpolateVideoRate *rate;
	InterpolateVideoRateMenu *rate_menu;
	InterpolateVideoKeyframes *keyframes;
};


PLUGIN_THREAD_HEADER(InterpolateVideo, InterpolateVideoThread, InterpolateVideoWindow)



class InterpolateVideo : public PluginVClient
{
public:
	InterpolateVideo(PluginServer *server);
	~InterpolateVideo();

	PLUGIN_CLASS_MEMBERS(InterpolateVideoConfig, InterpolateVideoThread)

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	void fill_border(double frame_rate, int64_t start_position);

// beginning and end frames
	VFrame *frames[2];
// Last requested positions
	int64_t frame_number[2];
// Last output position
	int64_t last_position;
	double last_rate;

// Current requested positions
	int64_t range_start;
	int64_t range_end;

};












InterpolateVideoConfig::InterpolateVideoConfig()
{
	input_rate = (double)30000 / 1001;
	use_keyframes = 0;
}

void InterpolateVideoConfig::copy_from(InterpolateVideoConfig *config)
{
	this->input_rate = config->input_rate;
	this->use_keyframes = config->use_keyframes;
}

int InterpolateVideoConfig::equivalent(InterpolateVideoConfig *config)
{
	return EQUIV(this->input_rate, config->input_rate) &&
		(this->use_keyframes == config->use_keyframes);
}









InterpolateVideoWindow::InterpolateVideoWindow(InterpolateVideo *plugin, int x, int y)
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

InterpolateVideoWindow::~InterpolateVideoWindow()
{
}

void InterpolateVideoWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, "Input frames per second:"));
	y += 30;
	add_subwindow(rate = new InterpolateVideoRate(plugin, 
		this, 
		x, 
		y));
	add_subwindow(rate_menu = new InterpolateVideoRateMenu(plugin, 
		this, 
		x + rate->get_w() + 5, 
		y));
	y += 30;
	add_subwindow(keyframes = new InterpolateVideoKeyframes(plugin,
		x, 
		y));

	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(InterpolateVideoWindow)












InterpolateVideoRate::InterpolateVideoRate(InterpolateVideo *plugin, 
	InterpolateVideoWindow *gui, 
	int x, 
	int y)
 : BC_TextBox(x, 
	y, 
	90,
	1,
	(float)plugin->config.input_rate)
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateVideoRate::handle_event()
{
	plugin->config.input_rate = Units::atoframerate(get_text());
	plugin->send_configure_change();
	return 1;
}




InterpolateVideoRateMenu::InterpolateVideoRateMenu(InterpolateVideo *plugin, 
	InterpolateVideoWindow *gui, 
	int x, 
	int y)
 : BC_ListBox(x,
 	y,
	100,
	200,
	LISTBOX_TEXT,
	&plugin->get_theme()->frame_rates,
	0,
	0,
	1,
	0,
	1)
{
	this->plugin = plugin;
	this->gui = gui;
}

int InterpolateVideoRateMenu::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	plugin->config.input_rate = atof(text);
	gui->rate->update(text);
	plugin->send_configure_change();
	return 1;
}




InterpolateVideoKeyframes::InterpolateVideoKeyframes(InterpolateVideo *plugin,
	int x, 
	int y)
 : BC_CheckBox(x, 
 	y, 
	plugin->config.use_keyframes, 
	"Use keyframes as input")
{
	this->plugin = plugin;
}
int InterpolateVideoKeyframes::handle_event()
{
	plugin->config.use_keyframes = get_value();
	plugin->send_configure_change();
	return 1;
}









PLUGIN_THREAD_OBJECT(InterpolateVideo, InterpolateVideoThread, InterpolateVideoWindow)










REGISTER_PLUGIN(InterpolateVideo)






InterpolateVideo::InterpolateVideo(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	bzero(frames, sizeof(VFrame*) * 2);
	for(int i = 0; i < 2; i++)
		frame_number[i] = -1;
	last_position = -1;
	last_rate = -1;
}


InterpolateVideo::~InterpolateVideo()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(frames[0]) delete frames[0];
	if(frames[1]) delete frames[1];
}


void InterpolateVideo::fill_border(double frame_rate, int64_t start_position)
{
// A border frame changed or the start position is identical to the last 
// start position.
	if(range_start != frame_number[0] || 
		last_position != start_position ||
		!EQUIV(last_rate, frame_rate))
	{
		read_frame(frames[0], 
			0, 
			range_start + (get_direction() == PLAY_REVERSE ? 1 : 0), 
			config.input_rate);
	}

	if(range_end != frame_number[1] || 
		last_position != start_position ||
		!EQUIV(last_rate, frame_rate))
	{
		read_frame(frames[1], 
			0, 
			range_end + (get_direction() == PLAY_REVERSE ? 1 : 0), 
			config.input_rate);
	}

	last_position = start_position;
	last_rate = frame_rate;
	frame_number[0] = range_start;
	frame_number[1] = range_end;
}


#define AVERAGE(type, components, max) \
{ \
	int fraction0 = (int)(lowest_fraction * max); \
	int fraction1 = (int)(highest_fraction * max); \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *in_row0 = (type*)frames[0]->get_rows()[i]; \
		type *in_row1 = (type*)frames[1]->get_rows()[i]; \
		type *out_row = (type*)frame->get_rows()[i]; \
		for(int j = 0; j < w * components; j++) \
		{ \
			*out_row++ = (*in_row0++ * fraction0 + *in_row1++ * fraction1) / max; \
		} \
	} \
}





int InterpolateVideo::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	if(get_direction() == PLAY_REVERSE) start_position--;
	load_configuration();

	if(!frames[0])
	{
		for(int i = 0; i < 2; i++)
		{
			frames[i] = new VFrame(0,
				frame->get_w(),
				frame->get_h(),
				frame->get_color_model(),
				-1);
		}
	}


	if(range_start == range_end)
	{
		read_frame(frame, 
			0, 
			range_start, 
			config.input_rate);
		return 0;
	}
	else
	{

// Fill border frames
		fill_border(frame_rate, start_position);

// Fraction of lowest frame in output
		int64_t requested_range_start = (int64_t)((double)range_start / 
			config.input_rate * 
			frame_rate);
		int64_t requested_range_end = (int64_t)((double)range_end / 
			config.input_rate * 
			frame_rate);
		float highest_fraction = (float)(start_position - requested_range_start) /
			(requested_range_end - requested_range_start);

// Fraction of highest frame in output
		float lowest_fraction = 1.0 - highest_fraction;

// printf("InterpolateVideo::process_buffer %f %f %lld %lld %lld %f %f\n",
// config.input_rate,
// frame_rate,
// requested_range_start,
// requested_range_end,
// start_position,
// lowest_fraction,
// highest_fraction);

		int w = frame->get_w();
		int h = frame->get_h();
		switch(frame->get_color_model())
		{
			case BC_RGB888:
			case BC_YUV888:
				AVERAGE(unsigned char, 3, 0xff);
				break;
			case BC_RGBA8888:
			case BC_YUVA8888:
				AVERAGE(unsigned char, 4, 0xff);
				break;
			case BC_RGB161616:
			case BC_YUV161616:
				AVERAGE(uint16_t, 3, 0xffff);
				break;
			case BC_RGBA16161616:
			case BC_YUVA16161616:
				AVERAGE(uint16_t, 4, 0xffff);
				break;
		}
	}
	return 0;
}



int InterpolateVideo::is_realtime()
{
	return 1;
}

char* InterpolateVideo::plugin_title()
{
	return _("Interpolate");
}

NEW_PICON_MACRO(InterpolateVideo) 

SHOW_GUI_MACRO(InterpolateVideo, InterpolateVideoThread)

RAISE_WINDOW_MACRO(InterpolateVideo)

SET_STRING_MACRO(InterpolateVideo)

int InterpolateVideo::load_configuration()
{
	KeyFrame *prev_keyframe, *next_keyframe;
	InterpolateVideoConfig old_config;
	old_config.copy_from(&config);

	next_keyframe = get_next_keyframe(get_source_position());
	prev_keyframe = get_prev_keyframe(get_source_position());
// Previous keyframe stays in config object.
	read_data(prev_keyframe);


	int64_t prev_position = edl_to_local(prev_keyframe->position);
	int64_t next_position = edl_to_local(next_keyframe->position);
	if(prev_position == 0 && next_position == 0)
	{
		next_position = prev_position = get_source_start();
	}
// printf("InterpolateVideo::load_configuration 1 %lld %lld %lld %lld\n",
// prev_keyframe->position,
// next_keyframe->position,
// prev_position,
// next_position);

// Get range to average in requested rate
	range_start = prev_position;
	range_end = next_position;


// Use keyframes to determine range
	if(config.use_keyframes)
	{
// Between keyframe and edge of range or no keyframes
		if(range_start == range_end)
		{
// Between first keyframe and start of effect
			if(get_source_position() >= get_source_start() &&
				get_source_position() < range_start)
			{
				range_start = get_source_start();
			}
			else
// Between last keyframe and end of effect
			if(get_source_position() >= range_start &&
				get_source_position() < get_source_start() + get_total_len())
			{
// Last frame should be inclusive of current effect
				range_end = get_source_start() + get_total_len() - 1;
			}
			else
			{
// Should never get here
				;
			}
		}

// Convert requested rate to input rate
		range_start = (int64_t)((double)range_start / get_framerate() * config.input_rate + 0.5);
		range_end = (int64_t)((double)range_end / get_framerate() * config.input_rate + 0.5);
	}
	else
// Use frame rate
	{
// Convert to input frame rate
		range_start = (int64_t)(get_source_position() / 
			get_framerate() *
			config.input_rate);
		range_end = (int64_t)(get_source_position() / 
			get_framerate() *
			config.input_rate) + 1;
	}

// printf("InterpolateVideo::load_configuration 1 %lld %lld %lld %lld %lld %lld\n",
// prev_keyframe->position,
// next_keyframe->position,
// prev_position,
// next_position,
// range_start,
// range_end);


	return !config.equivalent(&old_config);
}

int InterpolateVideo::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sinterpolatevideo.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.input_rate = defaults->get("INPUT_RATE", config.input_rate);
	config.input_rate = Units::fix_framerate(config.input_rate);
	config.use_keyframes = defaults->get("USE_KEYFRAMES", config.use_keyframes);
	return 0;
}

int InterpolateVideo::save_defaults()
{
	defaults->update("INPUT_RATE", config.input_rate);
	defaults->update("USE_KEYFRAMES", config.use_keyframes);
	defaults->save();
	return 0;
}

void InterpolateVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("INTERPOLATEVIDEO");
	output.tag.set_property("INPUT_RATE", config.input_rate);
	output.tag.set_property("USE_KEYFRAMES", config.use_keyframes);
	output.append_tag();
	output.terminate_string();
}

void InterpolateVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("INTERPOLATEVIDEO"))
		{
			config.input_rate = input.tag.get_property("INPUT_RATE", config.input_rate);
			config.input_rate = Units::fix_framerate(config.input_rate);
			config.use_keyframes = input.tag.get_property("USE_KEYFRAMES", config.use_keyframes);
		}
	}
}

void InterpolateVideo::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("InterpolateVideo::update_gui");
			thread->window->rate->update((float)config.input_rate);
			thread->window->keyframes->update(config.use_keyframes);
			thread->window->unlock_window();
		}
	}
}


