#include "bcdisplayinfo.h"
#include "defaults.h"
#include "filesystem.h"
#include "gainwindow.h"

#include <string.h>

PLUGIN_THREAD_OBJECT(Gain, GainThread, GainWindow)






GainWindow::GainWindow(Gain *gain, int x, int y)
 : BC_Window(gain->gui_string, 
 	x, 
	y, 
	230, 
	60, 
	230, 
	60, 
	0, 
	0,
	1)
{ 
	this->gain = gain; 
}

GainWindow::~GainWindow()
{
}

int GainWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(5, y, "Level:"));
	y += 20;
	add_tool(level = new GainLevel(gain, x, y));
	show_window();
	flush();
	return 0;
}

int GainWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}





GainLevel::GainLevel(Gain *gain, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	200,
	200,
	INFINITYGAIN, 
	40,
	gain->config.level)
{
	this->gain = gain;
}
int GainLevel::handle_event()
{
	gain->config.level = get_value();
	gain->send_configure_change();
	return 1;
}
