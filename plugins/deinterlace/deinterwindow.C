#include "bcdisplayinfo.h"
#include "deinterwindow.h"




PLUGIN_THREAD_OBJECT(DeInterlaceMain, DeInterlaceThread, DeInterlaceWindow)




DeInterlaceWindow::DeInterlaceWindow(DeInterlaceMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x, 
	y, 
	200, 
	300, 
	200, 
	300, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

DeInterlaceWindow::~DeInterlaceWindow()
{
}

int DeInterlaceWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, "Select lines to keep"));
	y += 25;
	add_tool(none = new DeInterlaceOption(client, this, DEINTERLACE_NONE, x, y, "Do nothing"));
	y += 25;
	add_tool(odd_fields = new DeInterlaceOption(client, this, DEINTERLACE_EVEN, x, y, "Odd lines"));
	y += 25;
	add_tool(even_fields = new DeInterlaceOption(client, this, DEINTERLACE_ODD, x, y, "Even lines"));
	y += 25;
	add_tool(average_fields = new DeInterlaceOption(client, this, DEINTERLACE_AVG, x, y, "Average lines"));
	y += 25;
	add_tool(swap_fields = new DeInterlaceOption(client, this, DEINTERLACE_SWAP, x, y, "Swap fields"));
	y += 25;
	add_tool(avg_even = new DeInterlaceOption(client, this, DEINTERLACE_AVG_EVEN, x, y, "Average even lines"));
	draw_line(170, y + 5, 190, y + 5);
	draw_line(190, y + 5, 190, y + 70);
	draw_line(150, y + 70, 190, y + 70);
	y += 25;
	add_tool(avg_odd = new DeInterlaceOption(client, this, DEINTERLACE_AVG_ODD, x, y, "Average odd lines"));
	draw_line(170, y + 5, 190, y + 5);
	y += 30;
	add_tool(adaptive = new DeInterlaceAdaptive(client, x, y));
	add_tool(threshold = new DeInterlaceThreshold(client, x + 100, y));
	y += 50;
	char string[BCTEXTLEN];
	get_status_string(string, 0);
	add_tool(status = new BC_Title(x, y, string));
	flash();
	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(DeInterlaceWindow)

void DeInterlaceWindow::get_status_string(char *string, int changed_rows)
{
	sprintf(string, "Changed rows: %d\n", changed_rows);
}

int DeInterlaceWindow::set_mode(int mode, int recursive)
{
	none->update(mode == DEINTERLACE_NONE);
	odd_fields->update(mode == DEINTERLACE_EVEN);
	even_fields->update(mode == DEINTERLACE_ODD);
	average_fields->update(mode == DEINTERLACE_AVG);
	swap_fields->update(mode == DEINTERLACE_SWAP);
	avg_even->update(mode == DEINTERLACE_AVG_EVEN);
	avg_odd->update(mode == DEINTERLACE_AVG_ODD);

	client->config.mode = mode;
	
	if(!recursive)
		client->send_configure_change();
	return 0;
}


DeInterlaceOption::DeInterlaceOption(DeInterlaceMain *client, 
		DeInterlaceWindow *window, 
		int output, 
		int x, 
		int y, 
		char *text)
 : BC_Radial(x, y, client->config.mode == output, text)
{
	this->client = client;
	this->window = window;
	this->output = output;
}

DeInterlaceOption::~DeInterlaceOption()
{
}
int DeInterlaceOption::handle_event()
{
	window->set_mode(output, 0);
	return 1;
}


DeInterlaceAdaptive::DeInterlaceAdaptive(DeInterlaceMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.adaptive, "Adaptive")
{
	this->client = client;
}
int DeInterlaceAdaptive::handle_event()
{
	client->config.adaptive = get_value();
	client->send_configure_change();
	return 1;
}



DeInterlaceThreshold::DeInterlaceThreshold(DeInterlaceMain *client, int x, int y)
 : BC_IPot(x, y, client->config.threshold, 0, 100)
{
	this->client = client;
}
int DeInterlaceThreshold::handle_event()
{
	client->config.threshold = get_value();
	client->send_configure_change();
	return 1;
}





