#include "huewindow.h"


HueThread::HueThread(HueMain *client)
 : Thread()
{
	this->client = client;
	synchronous = 1; // make thread wait for join
	gui_started.lock();
}

HueThread::~HueThread()
{
}
	
void HueThread::run()
{
	window = new HueWindow(client);
	window->create_objects();
	gui_started.unlock();
	window->run_window();
	delete window;
}






HueWindow::HueWindow(HueMain *client)
 : BC_Window("", MEGREY, client->gui_string, 210, 170, 200, 170, 0, !client->show_initially)
{ this->client = client; }

HueWindow::~HueWindow()
{
	delete hue_slider;
	delete saturation_slider;
	delete value_slider;
	delete automation[0];
	delete automation[1];
	delete automation[2];
}

int HueWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, "Hue"));
	add_tool(automation[0] = new AutomatedFn(client, this, x + 80, y, 0));
	y += 20;
	add_tool(hue_slider = new HueSlider(client, x, y));
	y += 35;
	add_tool(new BC_Title(x, y, "Saturation"));
	add_tool(automation[1] = new AutomatedFn(client, this, x + 80, y, 1));
	y += 20;
	add_tool(saturation_slider = new SaturationSlider(client, x, y));
	y += 35;
	add_tool(new BC_Title(x, y, "Value"));
	add_tool(automation[2] = new AutomatedFn(client, this, x + 80, y, 2));
	y += 20;
	add_tool(value_slider = new ValueSlider(client, x, y));
}

int HueWindow::close_event()
{
	client->save_defaults();
	hide_window();
	client->send_hide_gui();
}

HueSlider::HueSlider(HueMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->hue, -MAXHUE, MAXHUE, DKGREY, BLACK, 1)
{
	this->client = client;
}
HueSlider::~HueSlider()
{
}
int HueSlider::handle_event()
{
	client->hue = get_value();
	client->send_configure_change();
}

SaturationSlider::SaturationSlider(HueMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->saturation, -MAXVALUE, MAXVALUE, DKGREY, BLACK, 1)
{
	this->client = client;
}
SaturationSlider::~SaturationSlider()
{
}
int SaturationSlider::handle_event()
{
	client->saturation = get_value();
	client->send_configure_change();
}

ValueSlider::ValueSlider(HueMain *client, int x, int y)
 : BC_ISlider(x, y, 190, 30, 200, client->value, -MAXVALUE, MAXVALUE, DKGREY, BLACK, 1)
{
	this->client = client;
}
ValueSlider::~ValueSlider()
{
}
int ValueSlider::handle_event()
{
	client->value = get_value();
	client->send_configure_change();
}

AutomatedFn::AutomatedFn(HueMain *client, HueWindow *window, int x, int y, int number)
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
	for(int i = 0; i < 3; i++)
	{
		if(i != number) window->automation[i]->update(0);
	}
	update(1);
	client->automated_function = number;
	client->send_configure_change();
}

