// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2011-2012 Einar Rünkaru <einarrunkaru at gmail dot com>


#ifndef PLUGINMACROS_H
#define PLUGINMACROS_H
/*
 * Properties of a plugin:
 * PLUGIN_IS_AUDIO
 * PLUGIN_IS_VIDEO
 * PLUGIN_IS_TRANSITION
 * PLUGIN_IS_REALTIME
 * PLUGIN_IS_MULTICHANNEL
 * PLUGIN_IS_SYNTHESIS
 * PLUGIN_NOT_KEYFRAMEABLE
 * PLUGIN_USES_TMPFRAME
 * PLUGIN_USES_OPENGL
 * PLUGIN_STATUS_GUI
 * PLUGIN_CUSTOM_LOAD_CONFIGURATION
 * PLUGIN_CLASS class_name
 * PLUGIN_TITLE plugin_title
 * PLUGIN_GUI_CLASS class_name
 * PLUGIN_CONFIG_CLASS class_name
 * PLUGIN_THREAD_CLASS class_name
 * PLUGIN_GUI_CLASS class_name
 * PLUGIN_MAX_CHANNELS number
 */

#include "bchash.h"
#include "bcresources.h"
#include "keyframe.h"
#include "vframe.h"

class PLUGIN_CLASS;

#ifdef PLUGIN_CONFIG_CLASS
class PLUGIN_CONFIG_CLASS;
#define PLUGIN_CLASS_HAS_CONFIG_MEMBER \
	int load_configuration(); \
	PLUGIN_CONFIG_CLASS config;
#else
#define PLUGIN_CLASS_HAS_CONFIG_MEMBER
#endif  // config

#ifdef PLUGIN_GUI_CLASS
#ifdef PLUGIN_THREAD_CLASS
class PLUGIN_THREAD_CLASS;
#ifndef PLUGIN_IS_TRANSITION
#define PLUGIN_CLASS_UPDATE_GUI_MEMBER \
	void update_gui();
#else
#define PLUGIN_CLASS_UPDATE_GUI_MEMBER
#endif
#define PLUGIN_CLASS_HAS_THREAD_MEMBERS \
	void show_gui(); \
	void set_string(); \
	void raise_window(); \
	void hide_gui(); \
	int gui_open() { return !!thread; }; \
	PLUGIN_CLASS_UPDATE_GUI_MEMBER \
	PLUGIN_THREAD_CLASS *thread;
#else
#define PLUGIN_CLASS_HAS_THREAD_MEMBERS \
	int get_parameters();
#endif // plugin thread
class PLUGIN_GUI_CLASS;
#define PLUGIN_CLASS_USES_GUI_MEMBER
#else
#define PLUGIN_CLASS_USES_GUI_MEMBER \
	int uses_gui() { return 0; };
#define PLUGIN_CLASS_HAS_THREAD_MEMBERS
#endif // plugin gui

#ifdef PLUGIN_IS_TRANSITION
#define PLUGIN_CLASS_TRANSITION_MEMBER \
	int is_transition() { return 1; };
#undef PLUGIN_IS_REALTIME
#else
#define PLUGIN_CLASS_TRANSITION_MEMBER
#endif // plugin transition

#ifdef PLUGIN_IS_REALTIME
#define PLUGIN_CLASS_REALTIME_MEMBER \
	int is_realtime() { return 1; };
#else
#define PLUGIN_CLASS_REALTIME_MEMBER
#endif // realtime

#ifdef PLUGIN_IS_MULTICHANNEL
#define PLUGIN_CLASS_MULTICHANNEL_MEMBER \
	int is_multichannel() { return 1; };
#else
#define PLUGIN_CLASS_MULTICHANNEL_MEMBER
#endif // multichannel

#ifdef PLUGIN_IS_SYNTHESIS
#define PLUGIN_CLASS_SYNTHESIS_MEMBER \
	int is_synthesis() { return 1; };
#else
#define PLUGIN_CLASS_SYNTHESIS_MEMBER
#endif // synthesis

#ifdef PLUGIN_USES_TMPFRAME
#define PLUGIN_CLASS_APIVERSION_MEMBER \
	int api_version() { return 3; };
#else
#define PLUGIN_CLASS_APIVERSION_MEMBER
#endif

#ifdef PLUGIN_NOT_KEYFRAMEABLE
#define PLUGIN_CLASS_NOT_KEYFRAMEABLE_MEMBER \
	int not_keyframeable() { return 1; };
#else
#define PLUGIN_CLASS_NOT_KEYFRAMEABLE_MEMBER
#endif

#ifdef PLUGIN_USES_OPENGL
#define PLUGIN_CLASS_USES_OPENGL_MEMBER \
	int has_opengl_support() { return 1; };
#else
#define PLUGIN_CLASS_USES_OPENGL_MEMBER
#endif

#ifdef PLUGIN_MAX_CHANNELS
#define PLUGIN_CLASS_MAX_CHANNELS_MEMBER \
	int multi_max_channels() { return PLUGIN_MAX_CHANNELS; };
#else
#define PLUGIN_CLASS_MAX_CHANNELS_MEMBER
#endif

#ifdef PLUGIN_STATUS_GUI
#define PLUGIN_CLASS_STATUS_GUI_MEMBER \
	int has_status_gui() { return 1; };
#else
#define PLUGIN_CLASS_STATUS_GUI_MEMBER
#endif

#define PLUGIN_CLASS_MEMBERS \
	VFrame* new_picon(); \
	const char* plugin_title() { return PLUGIN_TITLE; }; \
	PLUGIN_CLASS_APIVERSION_MEMBER \
	PLUGIN_CLASS_HAS_CONFIG_MEMBER \
	PLUGIN_CLASS_TRANSITION_MEMBER \
	PLUGIN_CLASS_REALTIME_MEMBER \
	PLUGIN_CLASS_HAS_THREAD_MEMBERS \
	PLUGIN_CLASS_USES_GUI_MEMBER \
	PLUGIN_CLASS_MULTICHANNEL_MEMBER \
	PLUGIN_CLASS_SYNTHESIS_MEMBER \
	PLUGIN_CLASS_NOT_KEYFRAMEABLE_MEMBER \
	PLUGIN_CLASS_USES_OPENGL_MEMBER \
	PLUGIN_CLASS_MAX_CHANNELS_MEMBER \
	PLUGIN_CLASS_STATUS_GUI_MEMBER \
	unsigned char *picon_data; \
	BC_Hash *defaults;

#ifdef PLUGIN_CONFIG_CLASS
#define PLUGIN_CONFIG_CLASS_BASE_MEMBERS \
	ptstime prev_border_pts; \
	ptstime next_border_pts;
#ifdef PLUGIN_IS_AUDIO
#define PLUGIN_CONFIG_CLASS_MEMBERS \
	PLUGIN_CONFIG_CLASS_BASE_MEMBERS \
	ptstime slope_start, slope_end;
#else
#define PLUGIN_CONFIG_CLASS_MEMBERS \
	PLUGIN_CONFIG_CLASS_BASE_MEMBERS
#endif // is audio
#endif // config_class

#ifdef PLUGIN_GUI_CLASS
#define PLUGIN_GUI_CLASS_MEMBERS \
	PLUGIN_CLASS *plugin;
#endif

#ifdef PLUGIN_THREAD_CLASS
#define PLUGIN_THREAD_HEADER \
class PLUGIN_THREAD_CLASS : public Thread \
{ \
public: \
	PLUGIN_THREAD_CLASS(PLUGIN_CLASS *plugin); \
	~PLUGIN_THREAD_CLASS(); \
	void run(); \
	PLUGIN_GUI_CLASS *window; \
	PLUGIN_CLASS *plugin; \
};
#else
#define PLUGIN_THREAD_HEADER
#endif // plugin thread

#define REGISTER_PLUGIN \
PluginClient* new_plugin(PluginServer *server) \
{ \
	return new PLUGIN_CLASS(server); \
}

#ifdef PLUGIN_THREAD_CLASS
#define PLUGIN_CONSTRUCTOR_MACRO \
	thread = 0; \
	defaults = 0; \
	picon_data = picon_png; \
	load_defaults(); \
	load_configuration();

#define PLUGIN_DESTRUCTOR_MACRO \
	if(thread) \
	{ \
		if(!thread->window->window_done) \
		{ \
			thread->window->set_done(0); \
		} \
		delete thread; \
	} \
	delete defaults;
#else
#define PLUGIN_CONSTRUCTOR_MACRO \
	defaults = 0; \
	picon_data = picon_png; \
	load_defaults();

#define PLUGIN_DESTRUCTOR_MACRO \
	if(defaults) \
	{ \
		save_defaults(); \
		delete defaults; \
	}
#endif // gui

#define PLUGIN_CLASS_NEW_PICON \
VFrame* PLUGIN_CLASS::new_picon() \
{ \
	return new VFrame(picon_png); \
} \

#if !defined(PLUGIN_CUSTOM_LOAD_CONFIGURATION) && defined(PLUGIN_CONFIG_CLASS)
#define PLUGIN_CLASS_LOAD_CONFIGURATION \
int PLUGIN_CLASS::load_configuration() \
{ \
	KeyFrame *prev_keyframe, *next_keyframe; \
 \
	prev_keyframe = get_prev_keyframe(source_pts); \
	next_keyframe = get_next_keyframe(source_pts); \
 \
	if(!prev_keyframe || prev_keyframe == next_keyframe) \
		return get_need_reconfigure(); \
 \
	ptstime next_pts = next_keyframe->pos_time; \
	ptstime prev_pts = prev_keyframe->pos_time; \
 \
	PLUGIN_CONFIG_CLASS old_config, prev_config, next_config; \
	old_config.copy_from(config); \
	read_data(prev_keyframe); \
	if(PTSEQU(next_pts, prev_pts)) \
	{ \
		config.next_border_pts = get_end(); \
		if(PTSEQU(prev_pts, 0)) \
			config.prev_border_pts = get_start(); \
		else \
			config.prev_border_pts = prev_pts; \
		if(!config.equivalent(old_config)) \
			return 1; \
		return get_need_reconfigure(); \
	} \
	prev_config.copy_from(config); \
	read_data(next_keyframe); \
	next_config.copy_from(config); \
 \
	config.interpolate(prev_config,  \
		next_config,  \
		prev_pts, \
		next_pts, \
		source_pts); \
	config.next_border_pts = next_pts; \
	if(PTSEQU(prev_pts, 0)) \
		config.prev_border_pts = get_start(); \
	else \
		config.prev_border_pts = prev_pts; \
 \
	if(!config.equivalent(old_config)) \
		return 1; \
	else \
		return get_need_reconfigure(); \
}
#else
#define PLUGIN_CLASS_LOAD_CONFIGURATION
#endif

#ifdef PLUGIN_GUI_CLASS
#define PLUGIN_CLASS_SHOW_GUI \
void PLUGIN_CLASS::show_gui() \
{ \
	PLUGIN_THREAD_CLASS *new_thread = new PLUGIN_THREAD_CLASS(this); \
	new_thread->start(); \
} \
 \
void PLUGIN_CLASS::hide_gui() \
{ \
	PLUGIN_THREAD_CLASS *old_thread = thread; \
 \
	thread = 0; \
	if(old_thread && old_thread->window) \
		old_thread->window->close_event(); \
	delete old_thread; \
	if(defaults) \
		save_defaults(); \
}

#define PLUGIN_CLASS_SET_STRING \
void PLUGIN_CLASS::set_string() \
{ \
	if(thread) \
	{ \
		thread->window->set_utf8title(gui_string); \
	} \
}

#define PLUGIN_CLASS_RAISE_WINDOW \
void PLUGIN_CLASS::raise_window() \
{ \
	if(thread) \
	{ \
		thread->window->raise_window(); \
		thread->window->flush(); \
	} \
}

#ifndef PLUGIN_IS_TRANSITION
#define PLUGIN_CLASS_UPDATE_GUI \
void PLUGIN_CLASS::update_gui() \
{ \
	if(thread) \
		thread->window->update(); \
}
#else
#define PLUGIN_CLASS_UPDATE_GUI
#endif

#define PLUGIN_CLASS_GET_PARAMETERS \
int PLUGIN_CLASS::get_parameters() \
{ \
	int x, y; \
	BC_Resources::get_abs_cursor(&x, &y); \
	PLUGIN_GUI_CLASS window(this, x, y); \
	return window.run_window(); \
}
#else
#define PLUGIN_CLASS_GET_PARAMETERS
#endif // plugin gui

#if defined(PLUGIN_IS_REALTIME) || defined(PLUGIN_IS_TRANSITION)
#ifdef PLUGIN_THREAD_CLASS
#define PLUGIN_CLASS_METHODS \
	PLUGIN_CLASS_NEW_PICON \
	PLUGIN_CLASS_LOAD_CONFIGURATION \
	PLUGIN_CLASS_SHOW_GUI \
	PLUGIN_CLASS_SET_STRING \
	PLUGIN_CLASS_RAISE_WINDOW \
	PLUGIN_CLASS_UPDATE_GUI
#else
#define PLUGIN_CLASS_METHODS \
	PLUGIN_CLASS_NEW_PICON
#endif // plugin gui
#else
#define PLUGIN_CLASS_METHODS \
	PLUGIN_CLASS_GET_PARAMETERS \
	PLUGIN_CLASS_NEW_PICON
#endif // realtime or transition

#ifdef PLUGIN_THREAD_CLASS
#define PLUGIN_THREAD_METHODS \
PLUGIN_THREAD_CLASS::PLUGIN_THREAD_CLASS(PLUGIN_CLASS *plugin) \
 : Thread(THREAD_SYNCHRONOUS) \
{ \
	this->plugin = plugin; \
	window = 0; \
} \
 \
PLUGIN_THREAD_CLASS::~PLUGIN_THREAD_CLASS() \
{ \
	if(window) \
	{ \
		join(); \
		delete window; \
	} \
} \
 \
void PLUGIN_THREAD_CLASS::run() \
{ \
	int x, y; \
	BC_Resources::get_abs_cursor(&x, &y); \
	window = new PLUGIN_GUI_CLASS(plugin, \
		x - 75, y - 65); \
 \
/* Only set it here so tracking doesn't update it until everything is created. */ \
	plugin->thread = this; \
	int result = window->run_window(); \
	window->hide_window(); \
/* This is needed when the GUI is closed from itself */ \
	if(result) plugin->client_side_close(); \
}
#else
#define PLUGIN_THREAD_METHODS
#endif // thread_class

#ifdef PLUGIN_IS_AUDIO
#define PLUGIN_CONFIG_INTERPOLATE_MACRO \
	double next_scale = (current_pts - prev_pts) / (next_pts - prev_pts); \
	double prev_scale = (next_pts - current_pts) / (next_pts - prev_pts); \
	slope_start = prev_pts; \
	slope_end = next_pts;
#else
#define PLUGIN_CONFIG_INTERPOLATE_MACRO \
	double next_scale = (current_pts - prev_pts) / (next_pts - prev_pts); \
	double prev_scale = (next_pts - current_pts) / (next_pts - prev_pts);
#endif // is audio

#ifdef PLUGIN_GUI_CLASS
#if defined(PLUGIN_IS_REALTIME) || defined(PLUGIN_IS_TRANSITION)
#define PLUGIN_GUI_CONSTRUCTOR_MACRO \
	this->plugin = plugin; \
	set_icon(new VFrame(plugin->picon_data)); \
	update(); \
	show_window(); \
	flush();
#else
#define PLUGIN_GUI_CONSTRUCTOR_MACRO \
	this->plugin = plugin; \
	set_icon(new VFrame(plugin->picon_data)); \
	add_subwindow(new BC_OKButton(this)); \
	add_subwindow(new BC_CancelButton(this)); \
	update(); \
	show_window(); \
	flush();
#endif // is realtime or transition
#endif // plugin gui

#endif // pluginmacros
