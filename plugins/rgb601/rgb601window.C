#include "bcdisplayinfo.h"
#include "language.h"
#include "rgb601window.h"





PLUGIN_THREAD_OBJECT(RGB601Main, RGB601Thread, RGB601Window)






RGB601Window::RGB601Window(RGB601Main *client, int x, int y)
 : BC_Window(client->gui_string, 
	x,
	y,
	210, 
	200, 
	210, 
	200, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

RGB601Window::~RGB601Window()
{
}

int RGB601Window::create_objects()
{
	int x = 10, y = 10;
	
	add_tool(forward = new RGB601Direction(this, x, y, &client->config.direction, 1, _("RGB -> 601")));
	y += 30;
	add_tool(reverse = new RGB601Direction(this, x, y, &client->config.direction, 2, _("601 -> RGB")));

	show_window();
	flush();
	return 0;
}

void RGB601Window::update()
{
	forward->update(client->config.direction == 1);
	reverse->update(client->config.direction == 2);
}

WINDOW_CLOSE_EVENT(RGB601Window)

RGB601Direction::RGB601Direction(RGB601Window *window, int x, int y, int *output, int true_value, char *text)
 : BC_CheckBox(x, y, *output == true_value, text)
{
	this->output = output;
	this->true_value = true_value;
	this->window = window;
}
RGB601Direction::~RGB601Direction()
{
}
	
int RGB601Direction::handle_event()
{
	*output = get_value() ? true_value : 0;
	window->update();
	window->client->send_configure_change();
	return 1;
}

