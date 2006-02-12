#include "bcdisplayinfo.h"
#include "deinterwindow.h"
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



PLUGIN_THREAD_OBJECT(DeInterlaceMain, DeInterlaceThread, DeInterlaceWindow)




DeInterlaceWindow::DeInterlaceWindow(DeInterlaceMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x, 
	y, 
	400, 
	200, 
	400, 
	200, 
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
	add_tool(new BC_Title(x, y, _("Select deinterlacing mode")));
	y += 25;
	add_tool(mode = new DeInterlaceMode(client, this, x, y));
	mode->create_objects();
	y += 25;
	add_tool(dominance = new DeInterlaceDominance(client, x, y));
	y += 25;
	add_tool(adaptive = new DeInterlaceAdaptive(client, x, y));
	add_tool(threshold = new DeInterlaceThreshold(client, x + 150, y));
	y += 50;
	char string[BCTEXTLEN];
	get_status_string(string, 0);
	add_tool(status = new BC_Title(x, y, string));
	flash();
	show_window();
	set_mode(client->config.mode,0);
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(DeInterlaceWindow)

void DeInterlaceWindow::get_status_string(char *string, int changed_rows)
{
	sprintf(string, _("Changed rows: %d\n"), changed_rows);
}

int DeInterlaceWindow::set_mode(int mode, int recursive)
{
	client->config.mode = mode;
	dominance->update_mode(mode, client->config.dominance);
	adaptive->update_mode(mode, client->config.adaptive);
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
 : BC_CheckBox(x, y, client->config.adaptive, _("Setting Disabled                                 "))
{
	this->client = client;
}
int DeInterlaceAdaptive::handle_event()
{
	client->config.adaptive = get_value();
	client->send_configure_change();
	return 1;
}
void DeInterlaceAdaptive::update_mode (int mode, int adaptive_value)
{

	switch (client->config.mode) {
		case  DEINTERLACE_AVG:
			strcpy(this->caption,"Adaptive");
			this->enable();
			this->update(adaptive_value?BC_Toggle::TOGGLE_CHECKED:0);
			break;
		case DEINTERLACE_BOBWEAVE:
			strcpy(this->caption,"Adaptive");
			this->disable();
			this->update(TOGGLE_CHECKED);
			break;
		case DEINTERLACE_NONE:
		case DEINTERLACE_KEEP:
		case DEINTERLACE_AVG_1F:
		case DEINTERLACE_SWAP:
		case DEINTERLACE_TEMPORALSWAP:
		default:
			strcpy(this->caption,"Setting disabled");
			this->disable();
			this->update(0);

		break;
	}	

}

DeInterlaceDominance::DeInterlaceDominance(DeInterlaceMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.dominance, _("Keep Bottom fields (checked) or Top fields (unchecked)                  "))
{
	this->client = client;
}
int DeInterlaceDominance::handle_event()
{

	client->config.dominance = get_value();
	client->send_configure_change();
	return 1;
}

void DeInterlaceDominance::update_mode (int mode, int dominance_value)
{

	switch (mode) 
	{
		case DEINTERLACE_NONE:
		case  DEINTERLACE_AVG:
			strcpy(this->caption,"Setting disabled");
			this->disable();
			this->update(0);
			break;
		case DEINTERLACE_KEEP:
		case DEINTERLACE_BOBWEAVE:
			strcpy(this->caption,"Keep top (unchecked) or bottom (checked)");
			this->enable();
			this->update(dominance_value?BC_Toggle::TOGGLE_CHECKED:0);
			break;
		case DEINTERLACE_AVG_1F:
			strcpy(this->caption,"Average top (unchecked) or bottom (checked) fields");
			this->enable();
			this->update(dominance_value?BC_Toggle::TOGGLE_CHECKED:0);
			break;
		case DEINTERLACE_SWAP:
			strcpy(this->caption,"Swap top (unchecked) or bottom (checked) fields");
			this->enable();
			this->update(dominance_value?BC_Toggle::TOGGLE_CHECKED:0);
			break;
		case DEINTERLACE_TEMPORALSWAP:
			strcpy(this->caption,"Top (unchecked) or bottom (checked) field first");
			this->enable();
			this->update(dominance_value?BC_Toggle::TOGGLE_CHECKED:0);
			break;
		default:
			;
	}
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


DeInterlaceMode::DeInterlaceMode(DeInterlaceMain*plugin, 
	DeInterlaceWindow *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x, y, 200, to_text(plugin->config.mode), 1)
{
	this->plugin = plugin;
	this->gui = gui;
}
void DeInterlaceMode::create_objects()
{
	add_item(new BC_MenuItem(to_text(DEINTERLACE_NONE)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_KEEP)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_AVG)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_AVG_1F)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_BOBWEAVE)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_SWAP)));
	add_item(new BC_MenuItem(to_text(DEINTERLACE_TEMPORALSWAP)));
}

char* DeInterlaceMode::to_text(int mode)
{
	switch(mode)
	{
		case DEINTERLACE_KEEP:
			return _("Duplicate one field");
		case DEINTERLACE_AVG_1F:
			return _("Average one field");
		case DEINTERLACE_AVG:
			return _("Average both fields");
		case DEINTERLACE_BOBWEAVE:
			return _("Bob & Weave");
		case DEINTERLACE_SWAP:
			return _("Spatial field swap");
		case DEINTERLACE_TEMPORALSWAP:
			return _("Temporal field swap");
		default:
			return ("Do Nothing");
	}
}
int DeInterlaceMode::from_text(char *text)
{
	if(!strcmp(text, to_text(DEINTERLACE_KEEP))) 
		return DEINTERLACE_KEEP;
	if(!strcmp(text, to_text(DEINTERLACE_AVG))) 
		return DEINTERLACE_AVG;
	if(!strcmp(text, to_text(DEINTERLACE_AVG_1F))) 
		return DEINTERLACE_AVG_1F;
	if(!strcmp(text, to_text(DEINTERLACE_BOBWEAVE))) 
		return DEINTERLACE_BOBWEAVE;
	if(!strcmp(text, to_text(DEINTERLACE_SWAP))) 
		return DEINTERLACE_SWAP;
	if(!strcmp(text, to_text(DEINTERLACE_TEMPORALSWAP))) 
		return DEINTERLACE_TEMPORALSWAP;
	return DEINTERLACE_NONE;
}

int DeInterlaceMode::handle_event()
{
	plugin->config.mode = from_text(get_text());
	gui->set_mode(plugin->config.mode,0);
	plugin->send_configure_change();
}



