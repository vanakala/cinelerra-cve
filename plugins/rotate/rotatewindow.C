#include "rotatewindow.h"


RotateThread::RotateThread(RotateMain *client)
 : Thread()
{
	this->client = client;
	synchronous = 1; // make thread wait for join
	gui_started.lock();
}

RotateThread::~RotateThread()
{
}
	
void RotateThread::run()
{
	window = new RotateWindow(client);
	window->create_objects();
	gui_started.unlock();
	window->run_window();
	delete window;
}






RotateWindow::RotateWindow(RotateMain *client)
 : BC_Window("", MEGREY, client->gui_string, 250, 120, 250, 120, 0, !client->show_initially)
{ this->client = client; }

RotateWindow::~RotateWindow()
{
}

#define RADIUS 30

int RotateWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, "Rotate"));
	x += 50;
	y += 20;
	add_tool(toggle0 = new RotateToggle(this, client, client->angle == 0, x, y, 0, "0"));
    x += RADIUS;
    y += RADIUS;
	add_tool(toggle90 = new RotateToggle(this, client, client->angle == 90, x, y, 90, "90"));
    x -= RADIUS;
    y += RADIUS;
	add_tool(toggle180 = new RotateToggle(this, client, client->angle == 180, x, y, 180, "180"));
    x -= RADIUS;
    y -= RADIUS;
	add_tool(toggle270 = new RotateToggle(this, client, client->angle == 270, x, y, 270, "270"));
	x += 110;
	y -= 50;
	add_tool(fine = new RotateFine(this, client, x, y));
	y += fine->get_h() + 10;
	add_tool(new BC_Title(x, y, "Angle"));
	y += 20;
	add_tool(new BC_Title(x, y, "(Automated)"));
}

int RotateWindow::close_event()
{
	client->save_defaults();
	hide_window();
	client->send_hide_gui();
}

int RotateWindow::update_parameters()
{
	update_fine();
	update_toggles();
}

int RotateWindow::update_fine()
{
	fine->update(client->angle);
}

int RotateWindow::update_toggles()
{
	toggle0->update(client->angle == 0);
	toggle90->update(client->angle == 90);
	toggle180->update(client->angle == 180);
	toggle270->update(client->angle == 270);
}

RotateToggle::RotateToggle(RotateWindow *window, RotateMain *client, int init_value, int x, int y, int value, char *string)
 : BC_Radial(x, y, 16, 16, init_value, string)
{
	this->value = value;
	this->client = client;
    this->window = window;
}
RotateToggle::~RotateToggle()
{
}
int RotateToggle::handle_event()
{
	client->angle = (float)RotateToggle::value;
    window->update_parameters();
	client->send_configure_change();
}

RotateFine::RotateFine(RotateWindow *window, RotateMain *client, int x, int y)
 : BC_IPot(x, y, 40, 40, (int)client->angle, 0, 359, DKGREY, BLACK)
{
	this->window = window;
	this->client = client;
}

RotateFine::~RotateFine()
{
}

int RotateFine::handle_event()
{
	client->angle = get_value();
	window->update_toggles();
	client->send_configure_change();
}
