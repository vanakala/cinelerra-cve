#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "bcdisplayinfo.h"
#include "diffkeywindow.h"

//#define NO_GUI

PLUGIN_THREAD_OBJECT(DiffKeyMain, DiffKeyThread, DiffKeyWindow)


extern int grab_key_frame;
extern int add_key_frame;



DiffKeyWindow::DiffKeyWindow(DiffKeyMain *client, int x, int y)
 : BC_Window(client->get_gui_string(),
 	x,
	y,
	430, 
	310, // height was 360
	10,
	10,
#ifdef DEBUG
	1,
#endif
#ifndef DEBUG
	0,
#endif
	0,
	1)
{ 
	#ifdef DEBUG
		printf("\nConstructing window....");
		printf("\n");
		printf("\n");
	#endif

	this->client = client; 
}

DiffKeyWindow::~DiffKeyWindow()
{
	#ifdef DEBUG
		printf("\nDestroying window....");
		printf("\n");
		printf("\n");
	#endif
}









int DiffKeyWindow::create_objects()
{
#ifndef NO_GUI
	#ifdef DEBUG
		printf("\nPopulating window....");
		printf("\n");
		printf("\n");
	#endif

	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, "Hue sensitivity"));
	x += 170;
	add_tool(hue_on = new DiffKeyToggle(client, 
		&(client->config.hue_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(hue_imp = new DiffKeySlider(client,
		&(client->config.hue_imp),
		x,
		y));
	y += 30;
	add_tool(new BC_Title(x, y, "Saturation sensitivity"));
	x += 170;
	add_tool(sat_on = new DiffKeyToggle(client, 
		&(client->config.sat_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(sat_imp = new DiffKeySlider(client,
		&(client->config.sat_imp),
		x,
		y));
	y += 30;
	add_tool(new BC_Title(x, y, "Value sensitivity"));
	x += 170;
	add_tool(val_on = new DiffKeyToggle(client, 
		&(client->config.val_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(val_imp = new DiffKeySlider(client,
		&(client->config.val_imp),
		x,
		y));
	y += 30;
	add_tool(new BC_Title(x, y, "Visibility threshold"));
	x += 170;
	add_tool(vis_on = new DiffKeyToggle(client, 
		&(client->config.vis_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(vis_thresh = new DiffKeySlider(client,
		&(client->config.vis_thresh),
		x,
		y));
	y += 30;
	add_tool(new BC_Title(x, y, "Desaturation threshold"));
	x += 170;
	add_tool(desat_on = new DiffKeyToggle(client, 
		&(client->config.desat_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(desat_thresh = new DiffKeySlider(client,
		&(client->config.desat_thresh),
		x,
		y));
	y += 30;
/*	add_tool(add_key_frame = new DiffKeyAddButton(client,
		x,
		y));
	y += 40;*/// This button doesn't do anything yet.









	x = 220;
	y = 10;
	add_tool(new BC_Title(x, y, "Red sensitivity"));
	x += 170;
	add_tool(r_on = new DiffKeyToggle(client, 
		&(client->config.r_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(r_imp = new DiffKeySlider(client,
		&(client->config.r_imp),
		x,
		y));
	y += 30;
	add_tool(new BC_Title(x, y, "Green sensitivity"));
	x += 170;
	add_tool(g_on = new DiffKeyToggle(client, 
		&(client->config.g_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(g_imp = new DiffKeySlider(client,
		&(client->config.g_imp),
		x,
		y));
	y += 30;
	add_tool(new BC_Title(x, y, "Blue sensitivity"));
	x += 170;
	add_tool(b_on = new DiffKeyToggle(client, 
		&(client->config.b_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(b_imp = new DiffKeySlider(client,
		&(client->config.b_imp),
		x,
		y));
	y += 30;
	add_tool(new BC_Title(x, y, "Transparency threshold"));
	x += 170;
	add_tool(trans_on = new DiffKeyToggle(client, 
		&(client->config.trans_on), 
		"",
		x, 
		y));
	x -= 170;
	y += 30;
	add_tool(trans_thresh = new DiffKeySlider(client,
		&(client->config.trans_thresh),
		x,
		y));
	x += 25;
	y += 25; // was 30
	add_tool(reset_key_frame = new DiffKeyResetButton(client,
		x,
		y));
	x += 15;
	y += 30;
//	y += 30;
	add_tool(show_mask = new DiffKeyToggle(client, 
		&(client->config.show_mask), 
		"Show mask",
		x, 
		y));
	y += 40;




#endif

	show_window();
	flush();



}









int DiffKeyWindow::close_event()
{
	#ifdef DEBUG
		printf("\nClosing window....");
	#endif
	set_done(1);
	return 1;
}

DiffKeyToggle::DiffKeyToggle(DiffKeyMain *client, int *output, char *string, int x, int y)
 : BC_CheckBox(x, y, *output, string)
{
	#ifdef DEBUG
	printf("\nDiffKeyToggle::DiffKeyToggle");
	#endif
	this->client = client;
	this->output = output;
}
DiffKeyToggle::~DiffKeyToggle()
{
	#ifdef DEBUG
	printf("\nDiffKeyToggle::~DiffKeyToggle");
	#endif
}
int DiffKeyToggle::handle_event()
{
#ifdef DEBUG
	printf("\nToggle changed from %i", *output);
#endif
	*output = get_value();
	client->send_configure_change();
#ifdef DEBUG
	printf(" to %i", *output);
	printf("\n");
	printf("\n");
#endif
	return 1;
}


DiffKeyResetButton::DiffKeyResetButton(DiffKeyMain *client, int x, int y) : BC_GenericButton(x, y, "Reset key frame")
{
	#ifdef DEBUG
	printf("\nDiffKeyResetButton::DiffKeyResetButton");
	#endif
	this->client = client;
}

int DiffKeyResetButton::handle_event()
{
#ifdef DEBUG
	printf("\nReset frame button pressed.");
	printf("\ngrab_key_frame changed from %i", grab_key_frame);
#endif

	grab_key_frame = 1;
	client->send_configure_change();
#ifdef DEBUG
	printf(" to %i", grab_key_frame);
	printf("\n");
	printf("\n");
#endif
	return 1;
}


DiffKeyAddButton::DiffKeyAddButton(DiffKeyMain *client, int x, int y) : BC_GenericButton(x, y, "Add key frame")
{
	#ifdef DEBUG
	printf("\nDiffKeyAddButton::DiffKeyAddButton");
	#endif
	this->client = client;
}

int DiffKeyAddButton::handle_event()
{
#ifdef DEBUG
	printf("\nAdd frame button pressed.");
	printf("\nadd_key_frame changed from %i", add_key_frame);
#endif

	add_key_frame = 1;
	client->send_configure_change();
#ifdef DEBUG
	printf(" to %i", add_key_frame);
	printf("\n");
	printf("\n");
#endif
	return 1;
}


DiffKeySlider::DiffKeySlider(DiffKeyMain *client, 
	float *output, 
	int x, 
	int y)
 : BC_ISlider(x, 
 	y, 
	0, 
	200, 
	200,
	0, 
	100, 
	(int)*output)
{
	#ifdef DEBUG
	printf("\nDiffKeySlider::DiffKeySlider");
	#endif
	this->client = client;
	this->output = output;
}

int DiffKeySlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}
