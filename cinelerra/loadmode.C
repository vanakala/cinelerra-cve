#include "loadmode.h"
#include "mwindow.h"
#include "theme.h"


LoadModeItem::LoadModeItem(char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}


LoadMode::LoadMode(MWindow *mwindow,
	BC_WindowBase *window, 
	int x, 
	int y, 
	int *output, 
	int use_nothing)
{
	this->mwindow = mwindow;
	this->window = window;
	this->x = x;
	this->y = y;
	this->output = output;
	this->use_nothing = use_nothing;
	if(use_nothing) load_modes.append(new LoadModeItem("Insert nothing", LOAD_NOTHING));
	load_modes.append(new LoadModeItem("Replace current project", LOAD_REPLACE));
	load_modes.append(new LoadModeItem("Replace current project and concatenate tracks", LOAD_REPLACE_CONCATENATE));
	load_modes.append(new LoadModeItem("Append in new tracks", LOAD_NEW_TRACKS));
	load_modes.append(new LoadModeItem("Concatenate to existing tracks", LOAD_CONCATENATE));
	load_modes.append(new LoadModeItem("Paste at insertion point", LOAD_PASTE));
	load_modes.append(new LoadModeItem("Create new resources only", LOAD_RESOURCESONLY));
}

LoadMode::~LoadMode()
{
	delete title;
	delete textbox;
	delete listbox;
	for(int i = 0; i < load_modes.total; i++)
		delete load_modes.values[i];
}

char* LoadMode::mode_to_text()
{
	for(int i = 0; i < load_modes.total; i++)
	{
		if(load_modes.values[i]->value == *output) 
			return load_modes.values[i]->get_text();
	}
	return "Unknown";
}

int LoadMode::create_objects()
{
	int x = this->x, y = this->y;
	char *default_text;
	default_text = mode_to_text();

	window->add_subwindow(title = new BC_Title(x, y, "Insertion strategy:"));
	y += 20;
	window->add_subwindow(textbox = new BC_TextBox(x, 
		y, 
		mwindow->theme->loadmode_w, 
		1, 
		default_text));
	x += textbox->get_w();
	window->add_subwindow(listbox = new LoadModeListBox(window, this, x, y));

	return 0;
}

int LoadMode::reposition_window(int x, int y)
{
	title->reposition_window(x, y);
	y += 20;
	textbox->reposition_window(x, y);
	x += textbox->get_w();
	listbox->reposition_window(x, 
		y, 
		mwindow->theme->loadmode_w);
	return 0;
}


LoadModeListBox::LoadModeListBox(BC_WindowBase *window, 
	LoadMode *loadmode, 
	int x, 
	int y)
 : BC_ListBox(x,
 	y,
	loadmode->mwindow->theme->loadmode_w,
	150,
	LISTBOX_TEXT,
	(ArrayList<BC_ListBoxItem *>*)&loadmode->load_modes,
	0,
	0,
	1,
	0,
	1)
{
	this->window = window;
	this->loadmode = loadmode;
}

LoadModeListBox::~LoadModeListBox()
{
}

int LoadModeListBox::handle_event()
{
//printf("LoadModeListBox::handle_event 1 %d\n", get_selection(0, 0));
	if(get_selection(0, 0) >= 0)
	{
//printf("LoadModeListBox::handle_event 2 %s\n", get_selection(0, 0)->get_text());
		loadmode->textbox->update(get_selection(0, 0)->get_text());
		*(loadmode->output) = ((LoadModeItem*)get_selection(0, 0))->value;
	}
	return 1;
}





