#include "bcdisplayinfo.h"
#include "defaults.h"
#include "edl.inc"
#include "filexml.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"
#include "slide.h"


#include <stdint.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

REGISTER_PLUGIN(SlideMain)





SlideLeft::SlideLeft(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->motion_direction == 0, 
		_("Left"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideLeft::handle_event()
{
	update(1);
	plugin->motion_direction = 0;
	window->right->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideRight::SlideRight(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->motion_direction == 1, 
		_("Right"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideRight::handle_event()
{
	update(1);
	plugin->motion_direction = 1;
	window->left->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideIn::SlideIn(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 0, 
		_("In"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideIn::handle_event()
{
	update(1);
	plugin->direction = 0;
	window->out->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideOut::SlideOut(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 1, 
		_("Out"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideOut::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->in->update(0);
	plugin->send_configure_change();
	return 0;
}








SlideWindow::SlideWindow(SlideMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	320, 
	100, 
	320, 
	100, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}


int SlideWindow::close_event()
{
	set_done(1);
	return 1;
}

void SlideWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Direction:")));
	x += 100;
	add_subwindow(left = new SlideLeft(plugin, 
		this,
		x,
		y));
	x += 100;
	add_subwindow(right = new SlideRight(plugin, 
		this,
		x,
		y));

	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("Direction:")));
	x += 100;
	add_subwindow(in = new SlideIn(plugin, 
		this,
		x,
		y));
	x += 100;
	add_subwindow(out = new SlideOut(plugin, 
		this,
		x,
		y));

	show_window();
	flush();
}




PLUGIN_THREAD_OBJECT(SlideMain, SlideThread, SlideWindow)






SlideMain::SlideMain(PluginServer *server)
 : PluginVClient(server)
{
	motion_direction = 0;
	direction = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

SlideMain::~SlideMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* SlideMain::plugin_title() { return _("Slide"); }
int SlideMain::is_video() { return 1; }
int SlideMain::is_transition() { return 1; }
int SlideMain::uses_gui() { return 1; }
SHOW_GUI_MACRO(SlideMain, SlideThread);
SET_STRING_MACRO(SlideMain)
RAISE_WINDOW_MACRO(SlideMain)


VFrame* SlideMain::new_picon()
{
	return new VFrame(picon_png);
}

int SlideMain::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sslide.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	motion_direction = defaults->get("MOTION_DIRECTION", motion_direction);
	direction = defaults->get("DIRECTION", direction);
	return 0;
}

int SlideMain::save_defaults()
{
	defaults->update("MOTION_DIRECTION", motion_direction);
	defaults->update("DIRECTION", direction);
	defaults->save();
	return 0;
}

void SlideMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("SLIDE");
	output.tag.set_property("MOTION_DIRECTION", motion_direction);
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.terminate_string();
}

void SlideMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("SLIDE"))
		{
			motion_direction = input.tag.get_property("MOTION_DIRECTION", motion_direction);
			direction = input.tag.get_property("DIRECTION", direction);
		}
	}
}

void SlideMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
}






#define SLIDE(type, components) \
{ \
	if(direction == 0) \
	{ \
		if(motion_direction == 0) \
		{ \
			for(int j = 0; j < h; j++) \
			{ \
				type *in_row = (type*)incoming->get_rows()[j]; \
				type *out_row = (type*)outgoing->get_rows()[j]; \
				int x = w *  \
					PluginClient::get_source_position() /  \
					PluginClient::get_total_len(); \
	 \
				for(int k = 0, l = w - x; k < x; k++, l++) \
				{ \
					out_row[k * components + 0] = in_row[l * components + 0]; \
					out_row[k * components + 1] = in_row[l * components + 1]; \
					out_row[k * components + 2] = in_row[l * components + 2]; \
					if(components == 4) out_row[k * components + 3] = in_row[l * components + 3]; \
				} \
			} \
		} \
		else \
		{ \
			for(int j = 0; j < h; j++) \
			{ \
				type *in_row = (type*)incoming->get_rows()[j]; \
				type *out_row = (type*)outgoing->get_rows()[j]; \
				int x = w - w *  \
					PluginClient::get_source_position() /  \
					PluginClient::get_total_len(); \
	 \
				for(int k = x, l = 0; k < w; k++, l++) \
				{ \
					out_row[k * components + 0] = in_row[l * components + 0]; \
					out_row[k * components + 1] = in_row[l * components + 1]; \
					out_row[k * components + 2] = in_row[l * components + 2]; \
					if(components == 4) out_row[k * components + 3] = in_row[l * components + 3]; \
				} \
			} \
		} \
	} \
	else \
	{ \
		if(motion_direction == 0) \
		{ \
			for(int j = 0; j < h; j++) \
			{ \
				type *in_row = (type*)incoming->get_rows()[j]; \
				type *out_row = (type*)outgoing->get_rows()[j]; \
				int x = w - w *  \
					PluginClient::get_source_position() /  \
					PluginClient::get_total_len(); \
	 \
	 			int k, l; \
				for(k = 0, l = w - x; k < x; k++, l++) \
				{ \
					out_row[k * components + 0] = out_row[l * components + 0]; \
					out_row[k * components + 1] = out_row[l * components + 1]; \
					out_row[k * components + 2] = out_row[l * components + 2]; \
					if(components == 4) out_row[k * components + 3] = out_row[l * components + 3]; \
				} \
				for( ; k < w; k++) \
				{ \
					out_row[k * components + 0] = in_row[k * components + 0]; \
					out_row[k * components + 1] = in_row[k * components + 1]; \
					out_row[k * components + 2] = in_row[k * components + 2]; \
					if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
				} \
			} \
		} \
		else \
		{ \
			for(int j = 0; j < h; j++) \
			{ \
				type *in_row = (type*)incoming->get_rows()[j]; \
				type *out_row = (type*)outgoing->get_rows()[j]; \
				int x = w *  \
					PluginClient::get_source_position() /  \
					PluginClient::get_total_len(); \
	 \
				for(int k = w - 1, l = w - x - 1; k >= x; k--, l--) \
				{ \
					out_row[k * components + 0] = out_row[l * components + 0]; \
					out_row[k * components + 1] = out_row[l * components + 1]; \
					out_row[k * components + 2] = out_row[l * components + 2]; \
					if(components == 4) out_row[k * components + 3] = out_row[l * components + 3]; \
				} \
				for(int k = 0; k < x; k++) \
				{ \
					out_row[k * components + 0] = in_row[k * components + 0]; \
					out_row[k * components + 1] = in_row[k * components + 1]; \
					out_row[k * components + 2] = in_row[k * components + 2]; \
					if(components == 4) out_row[k * components + 3] = in_row[k * components + 3]; \
				} \
			} \
		} \
	} \
}





int SlideMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();


	switch(incoming->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			SLIDE(unsigned char, 3)
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			SLIDE(unsigned char, 4)
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			SLIDE(uint16_t, 3)
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			SLIDE(uint16_t, 4)
			break;
	}
	return 0;
}
