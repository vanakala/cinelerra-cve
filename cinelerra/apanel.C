#include "apanel.h"
#include "cwindowgui.h"

APanel::APanel(MWindow *mwindow, CWindowGUI *subwindow, int x, int y, int w, int h)
{
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
}

APanel::~APanel()
{
}

int APanel::create_objects()
{
	int x = this->x + 5, y = this->y + 10;
	char string[BCTEXTLEN];
	int x1 = x;

	subwindow->add_subwindow(new BC_Title(x, y, "Automation"));
	y += 20;
	for(int i = 0; i < PLUGINS; i++)
	{
		sprintf(string, "Plugin %d", i + 1);
		subwindow->add_subwindow(new BC_Title(x, y, string, SMALLFONT));
		subwindow->add_subwindow(plugin_autos[i] = new APanelPluginAuto(mwindow, this, x, y + 20));
		if(x == x1)
		{
			x += plugin_autos[i]->get_w();
		}
		else
		{
			x = x1;
			y += plugin_autos[i]->get_h() + 20;
		}
	}
	
	subwindow->add_subwindow(mute = new APanelMute(mwindow, this, x, y));
	y += mute->get_h();
	subwindow->add_subwindow(play = new APanelPlay(mwindow, this, x, y));
	return 0;
}




APanelPluginAuto::APanelPluginAuto(MWindow *mwindow, APanel *gui, int x, int y)
 : BC_FPot(x, 
		y, 
		0, 
		-1, 
		1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int APanelPluginAuto::handle_event()
{
	return 1;
}

APanelMute::APanelMute(MWindow *mwindow, APanel *gui, int x, int y)
 : BC_CheckBox(x, y, 0, "Mute")
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int APanelMute::handle_event()
{
	return 1;
}


APanelPlay::APanelPlay(MWindow *mwindow, APanel *gui, int x, int y)
 : BC_CheckBox(x, y, 1, "Play")
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int APanelPlay::handle_event()
{
	return 1;
}


