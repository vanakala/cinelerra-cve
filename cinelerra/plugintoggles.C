#include "mwindow.h"
#include "plugin.h"
#include "plugintoggles.h"
#include "theme.h"



PluginOn::PluginOn(MWindow *mwindow, int x, int y, Plugin *plugin)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->get_image_set("plugin_on"),
		plugin->on)
{
	this->mwindow = mwindow;
	this->plugin = plugin;
	in_use = 1;
}

int PluginOn::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("plugin_on")[0]->get_w();
}

void PluginOn::update(int x, int y, Plugin *plugin)
{
	BC_Toggle::set_value(plugin->on, 0);
	reposition_window(x, y);
	this->plugin = plugin;
	in_use = 1;
}

int PluginOn::handle_event()
{
	plugin->on = get_value();
	unlock_window();
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);
	lock_window("PluginOn::handle_event");
	return 1;
}




PluginShow::PluginShow(MWindow *mwindow, int x, int y, Plugin *plugin)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->get_image_set("plugin_show"),
		plugin->show)
{
	this->mwindow = mwindow;
	this->plugin = plugin;
	in_use = 1;
}

int PluginShow::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("plugin_show")[0]->get_w();
}

void PluginShow::update(int x, int y, Plugin *plugin)
{
	BC_Toggle::set_value(plugin->show, 0);
	reposition_window(x, y);
	this->plugin = plugin;
	in_use = 1;
}

int PluginShow::handle_event()
{
	unlock_window();
	if(get_value()) 
		mwindow->show_plugin(plugin);
	else
		mwindow->hide_plugin(plugin, 1);
	lock_window("PluginShow::handle_event");
	return 1;
}





