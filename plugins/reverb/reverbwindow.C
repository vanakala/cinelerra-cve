#include "bcdisplayinfo.h"
#include "defaults.h"
#include "filesystem.h"
#include "reverb.h"
#include "reverbwindow.h"

#include <string.h>

PLUGIN_THREAD_OBJECT(Reverb, ReverbThread, ReverbWindow)

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



ReverbWindow::ReverbWindow(Reverb *reverb, int x, int y)
 : BC_Window(reverb->gui_string, 
 	x, 
	y, 
	250, 
	230, 
	250, 
	230, 
	0, 
	0,
	1)
{ 
	this->reverb = reverb; 
}

ReverbWindow::~ReverbWindow()
{
}

int ReverbWindow::create_objects()
{
	int x = 170, y = 10;
	add_tool(new BC_Title(5, y + 10, _("Initial signal level:")));
	add_tool(level_init = new ReverbLevelInit(reverb, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("ms before reflections:")));
	add_tool(delay_init = new ReverbDelayInit(reverb, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("First reflection level:")));
	add_tool(ref_level1 = new ReverbRefLevel1(reverb, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Last reflection level:")));
	add_tool(ref_level2 = new ReverbRefLevel2(reverb, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Number of reflections:")));
	add_tool(ref_total = new ReverbRefTotal(reverb, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("ms of reflections:")));
	add_tool(ref_length = new ReverbRefLength(reverb, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Start band for lowpass:")));
	add_tool(lowpass1 = new ReverbLowPass1(reverb, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("End band for lowpass:")));
	add_tool(lowpass2 = new ReverbLowPass2(reverb, x + 35, y)); y += 40;
	show_window();
	flush();
	return 0;
}

int ReverbWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}





ReverbLevelInit::ReverbLevelInit(Reverb *reverb, int x, int y)
 : BC_FPot(x, 
 	y, 
	reverb->config.level_init, 
	INFINITYGAIN, 
	0)
{
	this->reverb = reverb;
}
ReverbLevelInit::~ReverbLevelInit() 
{
}
int ReverbLevelInit::handle_event()
{
//printf("ReverbLevelInit::handle_event 1 %p\n", reverb);
	reverb->config.level_init = get_value();
//printf("ReverbLevelInit::handle_event 1\n");
	reverb->send_configure_change();
//printf("ReverbLevelInit::handle_event 2\n");
	return 1;
}

ReverbDelayInit::ReverbDelayInit(Reverb *reverb, int x, int y)
 : BC_IPot(x, 
 	y, 
	reverb->config.delay_init, 
	0, 
	1000)
{
	this->reverb = reverb;
}
ReverbDelayInit::~ReverbDelayInit() 
{
}
int ReverbDelayInit::handle_event()
{
	reverb->config.delay_init = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbRefLevel1::ReverbRefLevel1(Reverb *reverb, int x, int y)
 : BC_FPot(x, 
 	y, 
	reverb->config.ref_level1, 
	INFINITYGAIN, 
	0)
{
	this->reverb = reverb;
}
ReverbRefLevel1::~ReverbRefLevel1() {}
int ReverbRefLevel1::handle_event()
{
	reverb->config.ref_level1 = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbRefLevel2::ReverbRefLevel2(Reverb *reverb, int x, int y)
 : BC_FPot(x, 
 	y, 
	reverb->config.ref_level2, 
	INFINITYGAIN, 
	0)
{
	this->reverb = reverb;
}
ReverbRefLevel2::~ReverbRefLevel2() {}
int ReverbRefLevel2::handle_event()
{
	reverb->config.ref_level2 = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbRefTotal::ReverbRefTotal(Reverb *reverb, int x, int y)
 : BC_IPot(x, 
 	y, 
	reverb->config.ref_total, 
	1, 
	250)
{
	this->reverb = reverb;
}
ReverbRefTotal::~ReverbRefTotal() {}
int ReverbRefTotal::handle_event()
{
	reverb->config.ref_total = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbRefLength::ReverbRefLength(Reverb *reverb, int x, int y)
 : BC_IPot(x, 
 	y, 
	reverb->config.ref_length, 
	0, 
	5000)
{
	this->reverb = reverb;
}
ReverbRefLength::~ReverbRefLength() {}
int ReverbRefLength::handle_event()
{
	reverb->config.ref_length = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbLowPass1::ReverbLowPass1(Reverb *reverb, int x, int y)
 : BC_QPot(x, 
 	y, 
	reverb->config.lowpass1)
{
	this->reverb = reverb;
}
ReverbLowPass1::~ReverbLowPass1() {}
int ReverbLowPass1::handle_event()
{
	reverb->config.lowpass1 = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbLowPass2::ReverbLowPass2(Reverb *reverb, int x, int y)
 : BC_QPot(x, 
 	y, 
	reverb->config.lowpass2)
{
	this->reverb = reverb;
}
ReverbLowPass2::~ReverbLowPass2() {}
int ReverbLowPass2::handle_event()
{
	reverb->config.lowpass2 = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbMenu::ReverbMenu(Reverb *reverb, ReverbWindow *window)
 : BC_MenuBar(0, 0, window->get_w())
{
	this->window = window;
	this->reverb = reverb;
}

ReverbMenu::~ReverbMenu()
{
	delete load;
	delete save;
	//delete set_default;
	for(int i = 0; i < total_loads; i++)
	{
		delete prev_load[i];
	}
	delete prev_load_thread;
}

int ReverbMenu::create_objects(Defaults *defaults)
{
	add_menu(filemenu = new BC_Menu(_("File")));
	filemenu->add_item(load = new ReverbLoad(reverb, this));
	filemenu->add_item(save = new ReverbSave(reverb, this));
	//filemenu->add_item(set_default = new ReverbSetDefault);
	load_defaults(defaults);
	prev_load_thread = new ReverbLoadPrevThread(reverb, this);
	return 0;
}

int ReverbMenu::load_defaults(Defaults *defaults)
{
	FileSystem fs;
	total_loads = defaults->get("TOTAL_LOADS", 0);
	if(total_loads > 0)
	{
		filemenu->add_item(new BC_MenuItem("-"));
		char string[1024], path[1024], filename[1024];
	
		for(int i = 0; i < total_loads; i++)
		{
			sprintf(string, "LOADPREVIOUS%d", i);
			defaults->get(string, path);
			fs.extract_name(filename, path);
//printf("ReverbMenu::load_defaults %s\n", path);
			filemenu->add_item(prev_load[i] = new ReverbLoadPrev(reverb, this, filename, path));
		}
	}
	return 0;
}

int ReverbMenu::save_defaults(Defaults *defaults)
{
	if(total_loads > 0)
	{
		defaults->update("TOTAL_LOADS",  total_loads);
		char string[1024];
		
		for(int i = 0; i < total_loads; i++)
		{
			sprintf(string, "LOADPREVIOUS%d", i);
			defaults->update(string, prev_load[i]->path);
		}
	}
	return 0;
}

int ReverbMenu::add_load(char *path)
{
	if(total_loads == 0)
	{
		filemenu->add_item(new BC_MenuItem("-"));
	}
	
// test for existing copy
	FileSystem fs;
	char text[1024], new_path[1024];      // get text and path
	fs.extract_name(text, path);
	strcpy(new_path, path);
	
	for(int i = 0; i < total_loads; i++)
	{
		if(!strcmp(prev_load[i]->get_text(), text))     // already exists
		{                                // swap for top load
			for(int j = i; j > 0; j--)   // move preceeding loads down
			{
				prev_load[j]->set_text(prev_load[j - 1]->get_text());
				prev_load[j]->set_path(prev_load[j - 1]->path);
			}
			prev_load[0]->set_text(text);
			prev_load[0]->set_path(new_path);
			return 1;
		}
	}
	
// add another load
	if(total_loads < TOTAL_LOADS)
	{
		filemenu->add_item(prev_load[total_loads] = new ReverbLoadPrev(reverb, this));
		total_loads++;
	}
	
// cycle loads down
	for(int i = total_loads - 1; i > 0; i--)
	{         
	// set menu item text
		prev_load[i]->set_text(prev_load[i - 1]->get_text());
	// set filename
		prev_load[i]->set_path(prev_load[i - 1]->path);
	}

// set up the new load
	prev_load[0]->set_text(text);
	prev_load[0]->set_path(new_path);
	return 0;
}

ReverbLoad::ReverbLoad(Reverb *reverb, ReverbMenu *menu)
 : BC_MenuItem(_("Load..."))
{
	this->reverb = reverb;
	this->menu = menu;
	thread = new ReverbLoadThread(reverb, menu);
}
ReverbLoad::~ReverbLoad()
{
	delete thread;
}
int ReverbLoad::handle_event()
{
	thread->start();
	return 0;
}

ReverbSave::ReverbSave(Reverb *reverb, ReverbMenu *menu)
 : BC_MenuItem(_("Save..."))
{
	this->reverb = reverb;
	this->menu = menu;
	thread = new ReverbSaveThread(reverb, menu);
}
ReverbSave::~ReverbSave()
{
	delete thread;
}
int ReverbSave::handle_event()
{
	thread->start();
	return 0;
}

ReverbSetDefault::ReverbSetDefault()
 : BC_MenuItem(_("Set default"))
{
}
int ReverbSetDefault::handle_event()
{
	return 0;
}

ReverbLoadPrev::ReverbLoadPrev(Reverb *reverb, ReverbMenu *menu, char *filename, char *path)
 : BC_MenuItem(filename)
{
	this->reverb = reverb;
	this->menu = menu;
	strcpy(this->path, path);
}
ReverbLoadPrev::ReverbLoadPrev(Reverb *reverb, ReverbMenu *menu)
 : BC_MenuItem("")
{
	this->reverb = reverb;
	this->menu = menu;
}
int ReverbLoadPrev::handle_event()
{
	menu->prev_load_thread->set_path(path);
	menu->prev_load_thread->start();
}
int ReverbLoadPrev::set_path(char *path)
{
	strcpy(this->path, path);
}


ReverbSaveThread::ReverbSaveThread(Reverb *reverb, ReverbMenu *menu)
 : Thread()
{
	this->reverb = reverb;
	this->menu = menu;
}
ReverbSaveThread::~ReverbSaveThread()
{
}
void ReverbSaveThread::run()
{
	int result = 0;
	{
		ReverbSaveDialog dialog(reverb);
		dialog.create_objects();
		result = dialog.run_window();
//		if(!result) strcpy(reverb->config_directory, dialog.get_path());
	}
	if(!result) 
	{
		result = reverb->save_to_file(reverb->config_directory);
		menu->add_load(reverb->config_directory);
	}
}

ReverbSaveDialog::ReverbSaveDialog(Reverb *reverb)
 : BC_FileBox(0,
 			0, 
 			reverb->config_directory, 
 			_("Save reverb"), 
 			_("Select the reverb file to save as"), 0, 0)
{
	this->reverb = reverb;
}
ReverbSaveDialog::~ReverbSaveDialog()
{
}
int ReverbSaveDialog::ok_event()
{
	set_done(0);
	return 0;
}
int ReverbSaveDialog::cancel_event()
{
	set_done(1);
	return 0;
}



ReverbLoadThread::ReverbLoadThread(Reverb *reverb, ReverbMenu *menu)
 : Thread()
{
	this->reverb = reverb;
	this->menu = menu;
}
ReverbLoadThread::~ReverbLoadThread()
{
}
void ReverbLoadThread::run()
{
	int result = 0;
	{
		ReverbLoadDialog dialog(reverb);
		dialog.create_objects();
		result = dialog.run_window();
//		if(!result) strcpy(reverb->config_directory, dialog.get_path());
	}
	if(!result) 
	{
		result = reverb->load_from_file(reverb->config_directory);
		if(!result)
		{
			menu->add_load(reverb->config_directory);
			reverb->send_configure_change();
		}
	}
}

ReverbLoadPrevThread::ReverbLoadPrevThread(Reverb *reverb, ReverbMenu *menu) : Thread()
{
	this->reverb = reverb;
	this->menu = menu;
}
ReverbLoadPrevThread::~ReverbLoadPrevThread()
{
}
void ReverbLoadPrevThread::run()
{
	int result = 0;
	strcpy(reverb->config_directory, path);
	result = reverb->load_from_file(path);
	if(!result)
	{
		menu->add_load(path);
		reverb->send_configure_change();
	}
}
int ReverbLoadPrevThread::set_path(char *path)
{
	strcpy(this->path, path);
	return 0;
}





ReverbLoadDialog::ReverbLoadDialog(Reverb *reverb)
 : BC_FileBox(0,
 			0, 
 			reverb->config_directory, 
 			_("Load reverb"), 
 			_("Select the reverb file to load from"), 0, 0)
{
	this->reverb = reverb;
}
ReverbLoadDialog::~ReverbLoadDialog()
{
}
int ReverbLoadDialog::ok_event()
{
	set_done(0);
	return 0;
}
int ReverbLoadDialog::cancel_event()
{
	set_done(1);
	return 0;
}

