#include "bcdisplayinfo.h"
#include "language.h"
#include "unsharp.h"
#include "unsharpwindow.h"






PLUGIN_THREAD_OBJECT(UnsharpMain, UnsharpThread, UnsharpWindow)



UnsharpWindow::UnsharpWindow(UnsharpMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	200, 
	160, 
	200, 
	160, 
	0, 
	1)
{
	this->plugin = plugin; 
}

UnsharpWindow::~UnsharpWindow()
{
}

int UnsharpWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y + 10, _("Radius:")));
	add_subwindow(radius = new UnsharpRadius(plugin, 
		x + title->get_w() + 10, 
		y));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Amount:")));
	add_subwindow(amount = new UnsharpAmount(plugin, 
		x + title->get_w() + 10, 
		y));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Threshold:")));
	add_subwindow(threshold = new UnsharpThreshold(plugin, 
		x + title->get_w() + 10, 
		y));

	show_window();
	flush();
	return 0;
}


void UnsharpWindow::update()
{
	radius->update(plugin->config.radius);
	amount->update(plugin->config.amount);
	threshold->update(plugin->config.threshold);
}


WINDOW_CLOSE_EVENT(UnsharpWindow)








UnsharpRadius::UnsharpRadius(UnsharpMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.radius, 0.1, 120)
{
	this->plugin = plugin;
}
int UnsharpRadius::handle_event()
{
	plugin->config.radius = get_value();
	plugin->send_configure_change();
	return 1;
}

UnsharpAmount::UnsharpAmount(UnsharpMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.amount, 0, 5)
{
	this->plugin = plugin;
}
int UnsharpAmount::handle_event()
{
	plugin->config.amount = get_value();
	plugin->send_configure_change();
	return 1;
}

UnsharpThreshold::UnsharpThreshold(UnsharpMain *plugin, int x, int y)
 : BC_IPot(x, y, plugin->config.threshold, 0, 255)
{
	this->plugin = plugin;
}
int UnsharpThreshold::handle_event()
{
	plugin->config.threshold = get_value();
	plugin->send_configure_change();
	return 1;
}




