#include "bcdisplayinfo.h"
#include "timeavgwindow.h"


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

PLUGIN_THREAD_OBJECT(TimeAvgMain, TimeAvgThread, TimeAvgWindow)





TimeAvgWindow::TimeAvgWindow(TimeAvgMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x, 
	y, 
	210, 
	80, 
	200, 
	80, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

TimeAvgWindow::~TimeAvgWindow()
{
}

int TimeAvgWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Frames to average")));
	y += 20;
	add_tool(total_frames = new TimeAvgSlider(client, x, y));
	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(TimeAvgWindow)

TimeAvgSlider::TimeAvgSlider(TimeAvgMain *client, int x, int y)
 : BC_ISlider(x, 
 	y, 
	0,
	190, 
	200, 
	1, 
	256, 
	client->config.frames)
{
	this->client = client;
}
TimeAvgSlider::~TimeAvgSlider()
{
}
int TimeAvgSlider::handle_event()
{
	int result = get_value();
	if(result < 1) result = 1;
	client->config.frames = result;
	client->send_configure_change();
	return 1;
}
