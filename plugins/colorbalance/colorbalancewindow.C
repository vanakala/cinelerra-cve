#include "bcdisplayinfo.h"
#include "colorbalancewindow.h"






PLUGIN_THREAD_OBJECT(ColorBalanceMain, ColorBalanceThread, ColorBalanceWindow)






ColorBalanceWindow::ColorBalanceWindow(ColorBalanceMain *client, int x, int y)
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

ColorBalanceWindow::~ColorBalanceWindow()
{
}

int ColorBalanceWindow::create_objects()
{
//printf("ColorBalanceWindow::create_objects 1\n");
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, "Color Balance"));
	y += 25;
//printf("ColorBalanceWindow::create_objects 1\n");
	add_tool(new BC_Title(x, y, "Cyan"));
	add_tool(cyan = new ColorBalanceSlider(client, &(client->config.cyan), x + 70, y));
	add_tool(new BC_Title(x + 270, y, "Red"));
	y += 25;
//printf("ColorBalanceWindow::create_objects 1\n");
	add_tool(new BC_Title(x, y, "Magenta"));
	add_tool(magenta = new ColorBalanceSlider(client, &(client->config.magenta), x + 70, y));
	add_tool(new BC_Title(x + 270, y, "Green"));
	y += 25;
//printf("ColorBalanceWindow::create_objects 1\n");
	add_tool(new BC_Title(x, y, "Yellow"));
	add_tool(yellow = new ColorBalanceSlider(client, &(client->config.yellow), x + 70, y));
	add_tool(new BC_Title(x + 270, y, "Blue"));
	y += 25;
//printf("ColorBalanceWindow::create_objects 1\n");
	add_tool(preserve = new ColorBalancePreserve(client, x + 70, y));
	y += 25;
//printf("ColorBalanceWindow::create_objects 1\n");
	add_tool(lock_params = new ColorBalanceLock(client, x + 70, y));
//printf("ColorBalanceWindow::create_objects 1\n");
	show_window();
//printf("ColorBalanceWindow::create_objects 2\n");
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(ColorBalanceWindow)

ColorBalanceSlider::ColorBalanceSlider(ColorBalanceMain *client, float *output, int x, int y)
 : BC_ISlider(x, 
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
    old_value = *output;
}
ColorBalanceSlider::~ColorBalanceSlider()
{
}
int ColorBalanceSlider::handle_event()
{
	float difference = get_value() - *output;
	*output = get_value();
    client->synchronize_params(this, difference);
	client->send_configure_change();
	return 1;
}

ColorBalancePreserve::ColorBalancePreserve(ColorBalanceMain *client, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.preserve, 
	"Preserve luminosity")
{
	this->client = client;
}
ColorBalancePreserve::~ColorBalancePreserve()
{
}

int ColorBalancePreserve::handle_event()
{
	client->config.preserve = get_value();
	client->send_configure_change();
	return 1;
}

ColorBalanceLock::ColorBalanceLock(ColorBalanceMain *client, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.lock_params, 
	"Lock parameters")
{
	this->client = client;
}
ColorBalanceLock::~ColorBalanceLock()
{
}

int ColorBalanceLock::handle_event()
{
	client->config.lock_params = get_value();
	client->send_configure_change();
	return 1;
}
