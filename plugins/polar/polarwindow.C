#include "polarwindow.h"


PLUGIN_THREAD_OBJECT(PolarMain, PolarThread, PolarWindow)







PolarWindow::PolarWindow(PolarMain *client)
 : BC_Window("", 
 	MEGREY, 
	client->gui_string, 
	210, 
	120, 
	200, 
	120, 
	0, 
	!client->show_initially)
{
	this->client = client; 
}

PolarWindow::~PolarWindow()
{
	delete depth_slider;
	delete angle_slider;
	delete automation[0];
	delete automation[1];
}

int PolarWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, "Depth"));
	add_tool(automation[0] = new AutomatedFn(client, this, x + 80, y, 0));
	y += 20;
	add_tool(depth_slider = new DepthSlider(client, x, y));
	y += 35;
	add_tool(new BC_Title(x, y, "Angle"));
	add_tool(automation[1] = new AutomatedFn(client, this, x + 80, y, 1));
	y += 20;
	add_tool(angle_slider = new AngleSlider(client, x, y));
}

int PolarWindow::close_event()
{
	client->save_defaults();
	hide_window();
	client->send_hide_gui();
}

DepthSlider::DepthSlider(PolarMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->depth, 0, MAXDEPTH, DKGREY, BLACK, 1)
{
	this->client = client;
}
DepthSlider::~DepthSlider()
{
}
int DepthSlider::handle_event()
{
	client->depth = get_value();
	client->send_configure_change();
}

AngleSlider::AngleSlider(PolarMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->angle, 0, MAXANGLE, DKGREY, BLACK, 1)
{
	this->client = client;
}
AngleSlider::~AngleSlider()
{
}
int AngleSlider::handle_event()
{
	client->angle = get_value();
	client->send_configure_change();
}

AutomatedFn::AutomatedFn(PolarMain *client, PolarWindow *window, int x, int y, int number)
 : BC_CheckBox(x, y, 16, 16, client->automated_function == number, "Automate")
{
	this->client = client;
	this->window = window;
	this->number = number;
}

AutomatedFn::~AutomatedFn()
{
}

int AutomatedFn::handle_event()
{
	for(int i = 0; i < 2; i++)
	{
		if(i != number) window->automation[i]->update(0);
	}
	update(1);
	client->automated_function = number;
	client->send_configure_change();
}

