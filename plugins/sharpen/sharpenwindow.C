#include "bcdisplayinfo.h"
#include "sharpenwindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


PLUGIN_THREAD_OBJECT(SharpenMain, SharpenThread, SharpenWindow)







SharpenWindow::SharpenWindow(SharpenMain *client, int x, int y)
 : BC_Window(client->gui_string, 
	x,
	y,
	210, 
	120, 
	210, 
	120, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

SharpenWindow::~SharpenWindow()
{
}

int SharpenWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Sharpness")));
	y += 20;
	add_tool(sharpen_slider = new SharpenSlider(client, &(client->config.sharpness), x, y));
	y += 30;
	add_tool(sharpen_interlace = new SharpenInterlace(client, x, y));
	y += 30;
	add_tool(sharpen_horizontal = new SharpenHorizontal(client, x, y));
	y += 30;
	add_tool(sharpen_luminance = new SharpenLuminance(client, x, y));
	show_window();
	flush();
	return 0;
}

int SharpenWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}

SharpenSlider::SharpenSlider(SharpenMain *client, float *output, int x, int y)
 : BC_ISlider(x, 
 	y, 
	0,
	200, 
	200,
	0, 
	MAXSHARPNESS, 
	(int)*output, 
	0, 
	0, 
	0)
{
	this->client = client;
	this->output = output;
}
SharpenSlider::~SharpenSlider()
{
}
int SharpenSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}




SharpenInterlace::SharpenInterlace(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.interlace, _("Interlace"))
{
	this->client = client;
}
SharpenInterlace::~SharpenInterlace()
{
}
int SharpenInterlace::handle_event()
{
	client->config.interlace = get_value();
	client->send_configure_change();
	return 1;
}




SharpenHorizontal::SharpenHorizontal(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.horizontal, _("Horizontal only"))
{
	this->client = client;
}
SharpenHorizontal::~SharpenHorizontal()
{
}
int SharpenHorizontal::handle_event()
{
	client->config.horizontal = get_value();
	client->send_configure_change();
	return 1;
}



SharpenLuminance::SharpenLuminance(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.luminance, _("Luminance only"))
{
	this->client = client;
}
SharpenLuminance::~SharpenLuminance()
{
}
int SharpenLuminance::handle_event()
{
	client->config.luminance = get_value();
	client->send_configure_change();
	return 1;
}

