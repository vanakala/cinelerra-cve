
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "clip.h"
#include "bchash.h"
#include "denoisemjpeg.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "picon_png.h"
#include "vframe.h"


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)




#include <stdint.h>
#include <string.h>



REGISTER_PLUGIN(DenoiseMJPEG)






DenoiseMJPEGConfig::DenoiseMJPEGConfig()
{
	radius = 8;
	threshold = 5;
	threshold2 = 4;
	sharpness = 125;
	lcontrast = 100;
	ccontrast = 100;
	deinterlace = 0;
	mode = 0;
	delay = 3;
}

int DenoiseMJPEGConfig::equivalent(DenoiseMJPEGConfig &that)
{
	return 
		that.radius == radius && 
		that.threshold == threshold && 
		that.threshold2 == threshold2 && 
		that.sharpness == sharpness && 
		that.lcontrast == lcontrast && 
		that.ccontrast == ccontrast && 
		that.deinterlace == deinterlace && 
		that.mode == mode && 
		that.delay == delay;
}

void DenoiseMJPEGConfig::copy_from(DenoiseMJPEGConfig &that)
{
	radius = that.radius;
	threshold = that.threshold;
	threshold2 = that.threshold2;
	sharpness = that.sharpness;
	lcontrast = that.lcontrast;
	ccontrast = that.ccontrast;
	deinterlace = that.deinterlace;
	mode = that.mode;
	delay = that.delay;
}

void DenoiseMJPEGConfig::interpolate(DenoiseMJPEGConfig &prev, 
	DenoiseMJPEGConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale);
	this->threshold = (int)(prev.threshold * prev_scale + next.threshold * next_scale);
	this->threshold2 = (int)(prev.threshold2 * prev_scale + next.threshold2 * next_scale);
	this->sharpness = (int)(prev.sharpness * prev_scale + next.sharpness * next_scale);
	this->lcontrast = (int)(prev.lcontrast * prev_scale + next.lcontrast * next_scale);
	this->ccontrast = (int)(prev.ccontrast * prev_scale + next.ccontrast * next_scale);
	this->deinterlace = prev.deinterlace;
	this->mode = prev.mode;
	this->delay = (int)(prev.delay * prev_scale + next.delay * next_scale);
}






DenoiseMJPEGRadius::DenoiseMJPEGRadius(DenoiseMJPEG *plugin, int x, int y)
 : BC_IPot(x, 
 	y, 
	plugin->config.radius,
	8,
	24)
{
	this->plugin = plugin;
}

int DenoiseMJPEGRadius::handle_event()
{
	int result = get_value();
	plugin->config.radius = result;
	plugin->send_configure_change();
	return 1;
}






DenoiseMJPEGThresh::DenoiseMJPEGThresh(DenoiseMJPEG *plugin, int x, int y)
 : BC_IPot(x, 
 	y, 
	plugin->config.threshold,
	0,
	255)
{
	this->plugin = plugin;
}

int DenoiseMJPEGThresh::handle_event()
{
	int result = get_value();
	plugin->config.threshold = result;
	plugin->send_configure_change();
	return 1;
}






DenoiseMJPEGThresh2::DenoiseMJPEGThresh2(DenoiseMJPEG *plugin, int x, int y)
 : BC_IPot(x, 
 	y, 
	plugin->config.threshold2,
	0,
	255)
{
	this->plugin = plugin;
}

int DenoiseMJPEGThresh2::handle_event()
{
	int result = get_value();
	plugin->config.threshold2 = result;
	plugin->send_configure_change();
	return 1;
}






DenoiseMJPEGSharp::DenoiseMJPEGSharp(DenoiseMJPEG *plugin, int x, int y)
 : BC_IPot(x, 
 	y, 
	plugin->config.sharpness,
	0,
	255)
{
	this->plugin = plugin;
}

int DenoiseMJPEGSharp::handle_event()
{
	int result = get_value();
	plugin->config.sharpness = result;
	plugin->send_configure_change();
	return 1;
}






DenoiseMJPEGLContrast::DenoiseMJPEGLContrast(DenoiseMJPEG *plugin, int x, int y)
 : BC_IPot(x, 
 	y, 
	plugin->config.lcontrast,
	0,
	255)
{
	this->plugin = plugin;
}

int DenoiseMJPEGLContrast::handle_event()
{
	int result = get_value();
	plugin->config.lcontrast = result;
	plugin->send_configure_change();
	return 1;
}






DenoiseMJPEGCContrast::DenoiseMJPEGCContrast(DenoiseMJPEG *plugin, int x, int y)
 : BC_IPot(x, 
 	y, 
	plugin->config.ccontrast,
	0,
	255)
{
	this->plugin = plugin;
}

int DenoiseMJPEGCContrast::handle_event()
{
	int result = get_value();
	plugin->config.ccontrast = result;
	plugin->send_configure_change();
	return 1;
}






DenoiseMJPEGDeinterlace::DenoiseMJPEGDeinterlace(DenoiseMJPEG *plugin, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	plugin->config.deinterlace,
	_("Deinterlace"))
{
	this->plugin = plugin;
}

int DenoiseMJPEGDeinterlace::handle_event()
{
	int result = get_value();
	plugin->config.deinterlace = result;
	plugin->send_configure_change();
	return 1;
}






DenoiseMJPEGModeProgressive::DenoiseMJPEGModeProgressive(DenoiseMJPEG *plugin, DenoiseMJPEGWindow *gui, int x, int y)
 : BC_Radial(x, 
 	y, 
	plugin->config.mode == 0,
	_("Progressive"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int DenoiseMJPEGModeProgressive::handle_event()
{
	int result = get_value();
	if(result) gui->update_mode(0);
	plugin->send_configure_change();
	return 1;
}


DenoiseMJPEGModeInterlaced::DenoiseMJPEGModeInterlaced(DenoiseMJPEG *plugin, DenoiseMJPEGWindow *gui, int x, int y)
 : BC_Radial(x, 
 	y, 
	plugin->config.mode == 1,
	_("Interlaced"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int DenoiseMJPEGModeInterlaced::handle_event()
{
	int result = get_value();
	if(result) gui->update_mode(1);
	plugin->send_configure_change();
	return 1;
}


DenoiseMJPEGModeFast::DenoiseMJPEGModeFast(DenoiseMJPEG *plugin, DenoiseMJPEGWindow *gui, int x, int y)
 : BC_Radial(x, 
 	y, 
	plugin->config.mode == 2,
	_("Fast"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int DenoiseMJPEGModeFast::handle_event()
{
	int result = get_value();
	if(result) gui->update_mode(2);
	plugin->send_configure_change();
	return 1;
}






DenoiseMJPEGDelay::DenoiseMJPEGDelay(DenoiseMJPEG *plugin, int x, int y)
 : BC_IPot(x, 
 	y, 
	plugin->config.delay,
	1,
	8)
{
	this->plugin = plugin;
}

int DenoiseMJPEGDelay::handle_event()
{
	int result = get_value();
	plugin->config.delay = result;
	plugin->send_configure_change();
	return 1;
}











DenoiseMJPEGWindow::DenoiseMJPEGWindow(DenoiseMJPEG *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	250, 
	350, 
	250, 
	350, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}


void DenoiseMJPEGWindow::create_objects()
{
	int x1 = 10, y1 = 20, x2 = 140, x3 = 180, y2 = 10, margin = 30, margin2 = 25;
	add_subwindow(new BC_Title(x1, y1, _("Search radius:")));
	add_subwindow(radius = new DenoiseMJPEGRadius(plugin, x2, y2));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y1, _("Pass 1 threshold:")));
	add_subwindow(threshold1 = new DenoiseMJPEGThresh(plugin, x3, y2));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y1, _("Pass 2 threshold:")));
	add_subwindow(threshold2 = new DenoiseMJPEGThresh2(plugin, x2, y2));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y1, _("Sharpness:")));
	add_subwindow(sharpness = new DenoiseMJPEGSharp(plugin, x3, y2));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y1, _("Luma contrast:")));
	add_subwindow(lcontrast = new DenoiseMJPEGLContrast(plugin, x2, y2));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y1, _("Chroma contrast:")));
	add_subwindow(ccontrast = new DenoiseMJPEGCContrast(plugin, x3, y2));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y1, _("Delay frames:")));
	add_subwindow(delay = new DenoiseMJPEGDelay(plugin, x2, y2));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y1, _("Mode:")));
	add_subwindow(interlaced = new DenoiseMJPEGModeInterlaced(plugin, this, x2, y1));
	y1 += margin2;
	y2 += margin2;
	add_subwindow(progressive = new DenoiseMJPEGModeProgressive(plugin, this, x2, y1));
	y1 += margin2;
	y2 += margin2;
	add_subwindow(fast = new DenoiseMJPEGModeFast(plugin, this, x2, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(deinterlace = new DenoiseMJPEGDeinterlace(plugin, x1, y1));

	show_window();
	flush();
}

void DenoiseMJPEGWindow::update_mode(int value)
{
	plugin->config.mode = value;
	progressive->update(value == 0);
	interlaced->update(value == 1);
	fast->update(value == 2);
}

int DenoiseMJPEGWindow::close_event()
{
	set_done(1);
	return 1;
}






PLUGIN_THREAD_OBJECT(DenoiseMJPEG, DenoiseMJPEGThread, DenoiseMJPEGWindow)











DenoiseMJPEG::DenoiseMJPEG(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	accumulation = 0;
}


DenoiseMJPEG::~DenoiseMJPEG()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(accumulation) delete [] accumulation;
}

int DenoiseMJPEG::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();

}

char* DenoiseMJPEG::plugin_title() { return N_("Denoise video2"); }
int DenoiseMJPEG::is_realtime() { return 1; }

VFrame* DenoiseMJPEG::new_picon()
{
	return new VFrame(picon_png);
}

SHOW_GUI_MACRO(DenoiseMJPEG, DenoiseMJPEGThread)

RAISE_WINDOW_MACRO(DenoiseMJPEG)

void DenoiseMJPEG::update_gui()
{
	if(thread) 
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->delay->update(config.delay);
		thread->window->threshold1->update(config.threshold);
		thread->window->unlock_window();
	}
}



SET_STRING_MACRO(DenoiseMJPEG);

LOAD_CONFIGURATION_MACRO(DenoiseMJPEG, DenoiseMJPEGConfig)

int DenoiseMJPEG::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sdenoisevideo.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.radius = defaults->get("RADIUS", config.radius);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.threshold2 = defaults->get("THRESHOLD2", config.threshold2);
	config.sharpness = defaults->get("SHARPNESS", config.sharpness);
	config.lcontrast = defaults->get("LCONTRAST", config.lcontrast);
	config.ccontrast = defaults->get("CCONTRAST", config.ccontrast);
	config.deinterlace = defaults->get("DEINTERLACE", config.deinterlace);
	config.mode = defaults->get("MODE", config.mode);
	config.delay = defaults->get("DELAY", config.delay);
	return 0;
}

int DenoiseMJPEG::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("THRESHOLD2", config.threshold2);
	defaults->update("SHARPNESS", config.sharpness);
	defaults->update("LCONTRAST", config.lcontrast);
	defaults->update("CCONTRAST", config.ccontrast);
	defaults->update("DEINTERLACE", config.deinterlace);
	defaults->update("MODE", config.mode);
	defaults->update("DELAY", config.delay);
	defaults->save();
	return 0;
}

void DenoiseMJPEG::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("DENOISE_VIDEO2");
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("THRESHOLD2", config.threshold2);
	output.tag.set_property("SHARPNESS", config.sharpness);
	output.tag.set_property("LCONTRAST", config.lcontrast);
	output.tag.set_property("CCONTRAST", config.ccontrast);
	output.tag.set_property("DEINTERLACE", config.deinterlace);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DELAY", config.delay);
	output.append_tag();
	output.tag.set_title("/DENOISE_VIDEO2");
	output.append_tag();
	output.terminate_string();
}

void DenoiseMJPEG::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("DENOISE_VIDEO2"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.threshold2 = input.tag.get_property("THRESHOLD2", config.threshold2);
			config.sharpness = input.tag.get_property("SHARPNESS", config.sharpness);
			config.lcontrast = input.tag.get_property("LCONTRAST", config.lcontrast);
			config.ccontrast = input.tag.get_property("CCONTRAST", config.ccontrast);
			config.deinterlace = input.tag.get_property("DEINTERLACE", config.deinterlace);
			config.mode = input.tag.get_property("MODE", config.mode);
			config.delay = input.tag.get_property("DELAY", config.delay);
		}
	}
}




