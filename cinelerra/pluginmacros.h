/*
 * pluginmacros.h
 * Copyright (C) 2011 Einar Rünkaru <einarry at smail dot ee>
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
 */


#ifndef PLUGINMACROS_H
#define PLUGINMACROS_H
/*
 * Parameters:
 * PLUGIN_IS_AUDIO
 * PLUGIN_IS_VIDEO
 * PLUGIN_IS_TRANSITION
 * PLUGIN_IS_REALTIME
 * PLUGIN_CUSTOM_LOAD_CONFIGURATION
 * PLUGIN_CLASS class_name
 * PLUGIN_TITLE plugin_title
 * PLUGIN_GUI_CLASS class_name
 * PLUGIN_CONFIG_CLASS class_name
 * PLUGIN_THREAD_CLASS class_name
 * PLUGIN_GUI_CLASS class_name
 */
class PLUGIN_CLASS;

#ifdef PLUGIN_CONFIG_CLASS
class PLUGIN_CONFIG_CLASS;
#endif

#ifdef PLUGIN_THREAD_CLASS
class PLUGIN_THREAD_CLASS;
#endif

#if defined(PLUGIN_IS_TRANSITION) && (defined(PLUGIN_THREAD_CLASS) || defined(PLUGIN_GUI_CLASS))
#error "Plugin must not have thread or gui defined"
#endif

#if defined(PLUGIN_THREAD_CLASS) && !defined(PLUGIN_GUI_CLASS)
#error "Thread has defined but no window!"
#endif

#if !defined(PLUGIN_IS_REALTIME) && defined(PLUGIN_THREAD_CLASS)
#define PLUGIN_IS_REALTIME
#endif

#ifdef PLUGIN_GUI_CLASS
class PLUGIN_GUI_CLASS;
#endif

#if defined(PLUGIN_IS_TRANSITION)
#define PLUGIN_CLASS_MEMBERS \
	VFrame* new_picon(); \
	const char* plugin_title() { return PLUGIN_TITLE; }; \
	int has_pts_api() { return 2; }; \
	int uses_gui() { return 0; }; \
	int is_transition() { return 1; };
#elif !defined(PLUGIN_IS_REALTIME)
#define PLUGIN_CLASS_MEMBERS \
	VFrame* new_picon(); \
	const char* plugin_title()  { return PLUGIN_TITLE; }; \
	BC_Hash *defaults; \
	int get_parameters(); \
	int has_pts_api() { return 2; };
#else
#ifndef PLUGIN_CONFIG_CLASS
#define PLUGIN_CLASS_MEMBERS \
	VFrame* new_picon(); \
	const char* plugin_title() { return PLUGIN_TITLE; }; \
	int uses_gui() { return 0; }; \
	int is_realtime() { return 1; }; \
	int has_pts_api() { return 2; };
#else
#define PLUGIN_CLASS_MEMBERS \
	int load_configuration(); \
	VFrame* new_picon(); \
	const char* plugin_title() { return PLUGIN_TITLE; }; \
	void show_gui(); \
	void set_string(); \
	void raise_window(); \
	void update_gui(); \
	BC_Hash *defaults; \
	PLUGIN_CONFIG_CLASS config; \
	PLUGIN_THREAD_CLASS *thread; \
	int is_realtime() { return 1; }; \
	int has_pts_api() { return 2; };
#endif // config_class
#endif // transition

#ifdef PLUGIN_CONFIG_CLASS
#ifdef PLUGIN_IS_AUDIO
#define PLUGIN_CONFIG_CLASS_MEMBERS \
	ptstime slope_start, slope_end;
#else
#define PLUGIN_CONFIG_CLASS_MEMBERS
#endif // is audio
#endif // config_class

#ifdef PLUGIN_GUI_CLASS
#define PLUGIN_GUI_CLASS_MEMBERS \
	PLUGIN_CLASS *plugin;
#endif

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

#define REGISTER_PLUGIN \
PluginClient* new_plugin(PluginServer *server) \
{ \
	return new PLUGIN_CLASS(server); \
}

#ifdef PLUGIN_IS_REALTIME
#ifdef PLUGIN_CONFIG_CLASS
#define PLUGIN_CONSTRUCTOR_MACRO \
	thread = 0; \
	defaults = 0; \
	load_defaults();

#define PLUGIN_DESTRUCTOR_MACRO \
	if(thread) \
	{ \
		thread->window->set_done(0); \
	} \
 \
	if(defaults) { \
		save_defaults(); \
		delete defaults; \
	}
#else
#define PLUGIN_CONSTRUCTOR_MACRO
#define PLUGIN_DESTRUCTOR_MACRO
#endif // config class
#elif defined(PLUGIN_IS_TRANSITION)
#define PLUGIN_CONSTRUCTOR_MACRO
#define PLUGIN_DESTRUCTOR_MACRO
#else // is transition
#define PLUGIN_CONSTRUCTOR_MACRO \
	defaults = 0; \
	load_defaults();

#define PLUGIN_DESTRUCTOR_MACRO \
	if(defaults) { \
		save_defaults(); \
		delete defaults; \
	}
#endif // is realtime


#define PLUGIN_CLASS_NEW_PICON \
VFrame* PLUGIN_CLASS::new_picon() \
{ \
	return new VFrame(picon_png); \
} \

#ifndef PLUGIN_CUSTOM_LOAD_CONFIGURATION
#define PLUGIN_CLASS_LOAD_CONFIGURATION \
int PLUGIN_CLASS::load_configuration() \
{ \
	KeyFrame *prev_keyframe, *next_keyframe; \
 \
	prev_keyframe = prev_keyframe_pts(source_pts); \
	next_keyframe = next_keyframe_pts(source_pts); \
 \
	ptstime next_pts = next_keyframe->pos_time; \
	ptstime prev_pts = prev_keyframe->pos_time; \
  \
	PLUGIN_CONFIG_CLASS old_config, prev_config, next_config; \
	old_config.copy_from(config); \
	read_data(prev_keyframe); \
	if(PTSEQU(next_pts, prev_pts)) \
		return 0; \
	prev_config.copy_from(config); \
	read_data(next_keyframe); \
	next_config.copy_from(config); \
 \
	config.interpolate(prev_config,  \
		next_config,  \
		prev_pts, \
		next_pts, \
		source_pts); \
 \
	if(!config.equivalent(old_config)) \
		return 1; \
	else \
		return 0; \
}
#else
#define PLUGIN_CLASS_LOAD_CONFIGURATION
#endif

#define PLUGIN_CLASS_SHOW_GUI \
void PLUGIN_CLASS::show_gui() \
{ \
	load_configuration(); \
	PLUGIN_THREAD_CLASS *new_thread = new PLUGIN_THREAD_CLASS(this); \
	new_thread->start(); \
}

#define PLUGIN_CLASS_SET_STRING \
void PLUGIN_CLASS::set_string() \
{ \
	if(thread) \
	{ \
		thread->window->set_title(gui_string); \
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

#define PLUGIN_CLASS_UPDATE_GUI \
void PLUGIN_CLASS::update_gui() \
{ \
	if(thread && load_configuration()) \
	{ \
		thread->window->lock_window("plugin::update_gui"); \
		thread->window->update(); \
		thread->window->unlock_window(); \
	} \
}

#define PLUGIN_CLASS_GET_PARAMETERS \
int PLUGIN_CLASS::get_parameters() \
{ \
	BC_DisplayInfo info; \
	PLUGIN_GUI_CLASS window(this, info.get_abs_cursor_x(), info.get_abs_cursor_y()); \
	return window.run_window(); \
}

#ifdef PLUGIN_IS_REALTIME
#ifdef PLUGIN_CONFIG_CLASS
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
#endif // plugin config
#elif defined(PLUGIN_IS_TRANSITION)
#define PLUGIN_CLASS_METHODS \
	PLUGIN_CLASS_NEW_PICON
#else
#define PLUGIN_CLASS_METHODS \
	PLUGIN_CLASS_GET_PARAMETERS \
	PLUGIN_CLASS_NEW_PICON
#endif

#ifdef PLUGIN_IS_REALTIME
#define PLUGIN_THREAD_METHODS \
PLUGIN_THREAD_CLASS::PLUGIN_THREAD_CLASS(PLUGIN_CLASS *plugin) \
 : Thread(0, 0, 1) \
{ \
	this->plugin = plugin; \
} \
 \
PLUGIN_THREAD_CLASS::~PLUGIN_THREAD_CLASS() \
{ \
	delete window; \
} \
 \
void PLUGIN_THREAD_CLASS::run() \
{ \
	BC_DisplayInfo info; \
	window = new PLUGIN_GUI_CLASS(plugin,  \
		info.get_abs_cursor_x() - 75,  \
		info.get_abs_cursor_y() - 65); \
 \
/* Only set it here so tracking doesn't update it until everything is created. */ \
	plugin->thread = this; \
	int result = window->run_window(); \
/* This is needed when the GUI is closed from itself */ \
	if(result) plugin->client_side_close(); \
	plugin->thread = 0; \
}
#endif // realtime

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
#ifdef PLUGIN_IS_REALTIME
#define PLUGIN_GUI_CONSTRUCTOR_MACRO \
	this->plugin = plugin; \
	VFrame *ico = plugin->new_picon(); \
	set_icon(ico); \
	delete ico; \
	show_window(); \
	flush();
#else
#define PLUGIN_GUI_CONSTRUCTOR_MACRO \
	this->plugin = plugin; \
	VFrame *ico = plugin->new_picon(); \
	set_icon(ico); \
	delete ico; \
	add_subwindow(new BC_OKButton(this)); \
	add_subwindow(new BC_CancelButton(this)); \
	show_window(); \
	flush();
#endif // is realtime
#endif // plugin gui

#endif // pluginmacros
