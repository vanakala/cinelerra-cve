#include "bcdisplayinfo.h"
#include "ivtcwindow.h"



PLUGIN_THREAD_OBJECT(IVTCMain, IVTCThread, IVTCWindow)






IVTCWindow::IVTCWindow(IVTCMain *client, int x, int y)
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

IVTCWindow::~IVTCWindow()
{
}

int IVTCWindow::create_objects()
{
	int x = 10, y = 10;
	static char *patterns[] = 
	{
		"A  B  BC  CD  D",
		"AB  BC  CD  DE  EF"
	};
	
	add_tool(new BC_Title(x, y, "Pattern offset:"));
	y += 20;
	add_tool(frame_offset = new IVTCOffset(client, x, y));
	y += 30;
	add_tool(first_field = new IVTCFieldOrder(client, x, y));
	y += 30;
	add_tool(automatic = new IVTCAuto(client, x, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, "Pattern:"));
	y += 20;
	for(int i = 0; i < TOTAL_PATTERNS; i++)
	{
		add_subwindow(pattern[i] = new IVTCPattern(client, 
			this, 
			i, 
			patterns[i], 
			x, 
			y));
		y += 20;
	}
	
//	y += 30;
//	add_tool(new BC_Title(x, y, "Field threshold:"));
//	y += 20;
//	add_tool(threshold = new IVTCAutoThreshold(client, x, y));
	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(IVTCWindow)

IVTCOffset::IVTCOffset(IVTCMain *client, int x, int y)
 : BC_TextBox(x, 
 	y, 
	190,
	1, 
	client->config.frame_offset)
{
	this->client = client;
}
IVTCOffset::~IVTCOffset()
{
}
int IVTCOffset::handle_event()
{
	client->config.frame_offset = atol(get_text());
	client->send_configure_change();
	return 1;
}




IVTCFieldOrder::IVTCFieldOrder(IVTCMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.first_field, "Odd field first")
{
	this->client = client;
}
IVTCFieldOrder::~IVTCFieldOrder()
{
}
int IVTCFieldOrder::handle_event()
{
	client->config.first_field = get_value();
	client->send_configure_change();
	return 1;
}


IVTCAuto::IVTCAuto(IVTCMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.automatic, "Automatic IVTC")
{
	this->client = client;
}
IVTCAuto::~IVTCAuto()
{
}
int IVTCAuto::handle_event()
{
	client->config.automatic = get_value();
	client->send_configure_change();
	return 1;
}

IVTCPattern::IVTCPattern(IVTCMain *client, 
	IVTCWindow *window, 
	int number, 
	char *text, 
	int x, 
	int y)
 : BC_Radial(x, y, client->config.pattern == number, text)
{
	this->window = window;
	this->client = client;
	this->number = number;
}
IVTCPattern::~IVTCPattern()
{
}
int IVTCPattern::handle_event()
{
	for(int i = 0; i < TOTAL_PATTERNS; i++)
	{
		if(i != number) window->pattern[i]->update(0);
	}
	client->config.pattern = number;
	update(1);
	client->send_configure_change();
	return 1;
}



IVTCAutoThreshold::IVTCAutoThreshold(IVTCMain *client, int x, int y)
 : BC_TextBox(x, y, 190, 1, client->config.auto_threshold)
{
	this->client = client;
}
IVTCAutoThreshold::~IVTCAutoThreshold()
{
}
int IVTCAutoThreshold::handle_event()
{
	client->config.auto_threshold = atof(get_text());
	client->send_configure_change();
	return 1;
}

