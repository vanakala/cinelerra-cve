#include "bcdisplayinfo.h"
#include "clip.h"
#include "translatewin.h"







PLUGIN_THREAD_OBJECT(TranslateMain, TranslateThread, TranslateWin)






TranslateWin::TranslateWin(TranslateMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x,
	y,
	300, 
	220, 
	300, 
	220, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

TranslateWin::~TranslateWin()
{
}

int TranslateWin::create_objects()
{
	int x = 10, y = 10;

	add_tool(new BC_Title(x, y, "In X:"));
	y += 20;
	in_x = new TranslateCoord(this, client, x, y, &client->config.in_x);
	in_x->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, "In Y:"));
	y += 20;
	in_y = new TranslateCoord(this, client, x, y, &client->config.in_y);
	in_y->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, "In W:"));
	y += 20;
	in_w = new TranslateCoord(this, client, x, y, &client->config.in_w);
	in_w->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, "In H:"));
	y += 20;
	in_h = new TranslateCoord(this, client, x, y, &client->config.in_h);
	in_h->create_objects();
	y += 30;


	x += 150;
	y = 10;
	add_tool(new BC_Title(x, y, "Out X:"));
	y += 20;
	out_x = new TranslateCoord(this, client, x, y, &client->config.out_x);
	out_x->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, "Out Y:"));
	y += 20;
	out_y = new TranslateCoord(this, client, x, y, &client->config.out_y);
	out_y->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, "Out W:"));
	y += 20;
	out_w = new TranslateCoord(this, client, x, y, &client->config.out_w);
	out_w->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, "Out H:"));
	y += 20;
	out_h = new TranslateCoord(this, client, x, y, &client->config.out_h);
	out_h->create_objects();
	y += 30;



	show_window();
	flush();
	return 0;
}

int TranslateWin::close_event()
{
	set_done(1);
	return 1;
}

TranslateCoord::TranslateCoord(TranslateWin *win, 
	TranslateMain *client, 
	int x, 
	int y,
	float *value)
 : BC_TumbleTextBox(win,
 	*value,
	0,
	100,
	x, 
	y, 
	100)
{
//printf("TranslateWidth::TranslateWidth %f\n", client->config.w);
	this->client = client;
	this->win = win;
	this->value = value;
}

TranslateCoord::~TranslateCoord()
{
}

int TranslateCoord::handle_event()
{
	*value = atof(get_text());

	client->send_configure_change();
	return 1;
}


