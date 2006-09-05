#include "bcdisplayinfo.h"
#include "brightnesswindow.h"
#include "language.h"


PLUGIN_THREAD_OBJECT(BrightnessMain, BrightnessThread, BrightnessWindow)





BrightnessWindow::BrightnessWindow(BrightnessMain *client, int x, int y)
 : BC_Window(client->gui_string, x,
 	y,
	330, 
	160, 
	330, 
	160, 
	0, 
	0)
{ 
	this->client = client; 
}

BrightnessWindow::~BrightnessWindow()
{
}

int BrightnessWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Brightness/Contrast")));
	y += 25;
	add_tool(new BC_Title(x, y,_("Brightness:")));
	add_tool(brightness = new BrightnessSlider(client, 
		&(client->config.brightness), 
		x + 80, 
		y,
		1));
	y += 25;
	add_tool(new BC_Title(x, y, _("Contrast:")));
	add_tool(contrast = new BrightnessSlider(client, 
		&(client->config.contrast), 
		x + 80, 
		y,
		0));
	y += 30;
	add_tool(luma = new BrightnessLuma(client, 
		x, 
		y));
	show_window();
	flush();
	return 0;
}

int BrightnessWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}

BrightnessSlider::BrightnessSlider(BrightnessMain *client, 
	float *output, 
	int x, 
	int y,
	int is_brightness)
 : BC_FSlider(x, 
 	y, 
	0, 
	200, 
	200,
	-100, 
	100, 
	(int)*output)
{
	this->client = client;
	this->output = output;
	this->is_brightness = is_brightness;
}
BrightnessSlider::~BrightnessSlider()
{
}
int BrightnessSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}

char* BrightnessSlider::get_caption()
{
	float fraction;
	if(is_brightness)
	{
		fraction = *output / 100;
	}
	else
	{
		fraction = (*output < 0) ? 
			(*output + 100) / 100 : 
			(*output + 25) / 25;
	}
	sprintf(string, "%0.4f", fraction);
	return string;
}


BrightnessLuma::BrightnessLuma(BrightnessMain *client, 
	int x, 
	int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.luma,
	_("Boost luminance only"))
{
	this->client = client;
}
BrightnessLuma::~BrightnessLuma()
{
}
int BrightnessLuma::handle_event()
{
	client->config.luma = get_value();
	client->send_configure_change();
	return 1;
}
