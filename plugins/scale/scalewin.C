#include "bcdisplayinfo.h"
#include "clip.h"
#include "scale.h"





PLUGIN_THREAD_OBJECT(ScaleMain, ScaleThread, ScaleWin)








ScaleWin::ScaleWin(ScaleMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x,
	y,
	150, 
	150, 
	150, 
	150, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

ScaleWin::~ScaleWin()
{
}

int ScaleWin::create_objects()
{
	int x = 10, y = 10;

	add_tool(new BC_Title(x, y, "X Scale:"));
	y += 20;
	width = new ScaleWidth(this, client, x, y);
	width->create_objects();
	y += 30;
	add_tool(new BC_Title(x, y, "Y Scale:"));
	y += 20;
	height = new ScaleHeight(this, client, x, y);
	height->create_objects();
	y += 35;
	add_tool(constrain = new ScaleConstrain(client, x, y));
	show_window();
	flush();
	return 0;
}

int ScaleWin::close_event()
{
	set_done(1);
	return 1;
}

ScaleWidth::ScaleWidth(ScaleWin *win, 
	ScaleMain *client, 
	int x, 
	int y)
 : BC_TumbleTextBox(win,
 	client->config.w,
	0,
	100,
	x, 
	y, 
	100)
{
//printf("ScaleWidth::ScaleWidth %f\n", client->config.w);
	this->client = client;
	this->win = win;
}

ScaleWidth::~ScaleWidth()
{
}

int ScaleWidth::handle_event()
{
	client->config.w = atof(get_text());
	CLAMP(client->config.w, 0, 100);

	if(client->config.constrain)
	{
		client->config.h = client->config.w;
		win->height->update(client->config.h);
	}

//printf("ScaleWidth::handle_event 1 %f\n", client->config.w);
	client->send_configure_change();
	return 1;
}




ScaleHeight::ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_TumbleTextBox(win,
 	client->config.h,
	0,
	100,
	x, 
	y, 
	100)
{
	this->client = client;
	this->win = win;
}
ScaleHeight::~ScaleHeight()
{
}
int ScaleHeight::handle_event()
{
	client->config.h = atof(get_text());
	CLAMP(client->config.h, 0, 100);

	if(client->config.constrain)
	{
		client->config.w = client->config.h;
		win->width->update(client->config.w);
	}

	client->send_configure_change();
	return 1;
}






ScaleConstrain::ScaleConstrain(ScaleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.constrain, "Constrain ratio")
{
	this->client = client;
}
ScaleConstrain::~ScaleConstrain()
{
}
int ScaleConstrain::handle_event()
{
	client->config.constrain = get_value();
	client->send_configure_change();
}
