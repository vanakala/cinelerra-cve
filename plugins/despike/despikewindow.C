#include "bcdisplayinfo.h"
#include "despikewindow.h"

#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


PLUGIN_THREAD_OBJECT(Despike, DespikeThread, DespikeWindow)






DespikeWindow::DespikeWindow(Despike *despike, int x, int y)
 : BC_Window(despike->gui_string, 
 	x, 
	y, 
	230, 
	110, 
	230, 
	110, 
	0, 
	0,
	1)
{ 
	this->despike = despike; 
}

DespikeWindow::~DespikeWindow()
{
}

int DespikeWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(5, y, _("Maximum level:")));
	y += 20;
	add_tool(level = new DespikeLevel(despike, x, y));
	y += 30;
	add_tool(new BC_Title(5, y, _("Maximum rate of change:")));
	y += 20;
	add_tool(slope = new DespikeSlope(despike, x, y));
	show_window();
	flush();
	return 0;
}

int DespikeWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}





DespikeLevel::DespikeLevel(Despike *despike, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	200,
	200,
	INFINITYGAIN, 
	0,
	despike->config.level)
{
	this->despike = despike;
}
int DespikeLevel::handle_event()
{
	despike->config.level = get_value();
	despike->send_configure_change();
	return 1;
}

DespikeSlope::DespikeSlope(Despike *despike, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	200,
	200,
	INFINITYGAIN, 
	0,
	despike->config.slope)
{
	this->despike = despike;
}
int DespikeSlope::handle_event()
{
	despike->config.slope = get_value();
	despike->send_configure_change();
	return 1;
}
