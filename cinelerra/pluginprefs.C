#include "pluginprefs.h"
#include "preferences.h"
#include <string.h>

PluginPrefs::PluginPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
}

PluginPrefs::~PluginPrefs()
{
	delete ipath;
	delete ipathtext;
	delete lpath;
	delete lpathtext;
}

int PluginPrefs::create_objects()
{
	char string[1024];
	int x = 5, y = 5;

// 	add_border(get_resources()->get_bg_shadow1(),
// 		get_resources()->get_bg_shadow2(),
// 		get_resources()->get_bg_color(),
// 		get_resources()->get_bg_light2(),
// 		get_resources()->get_bg_light1());

	add_subwindow(new BC_Title(x, y, "Plugin Set", LARGEFONT, BLACK));
	y += 35;
	add_subwindow(new BC_Title(x, y, "Look for global plugins here", MEDIUMFONT, BLACK));
	y += 20;
	add_subwindow(ipathtext = new PluginGlobalPathText(x, y, pwindow, pwindow->thread->preferences->global_plugin_dir));
	add_subwindow(ipath = new BrowseButton(mwindow,
		this,
		ipathtext, 
		215, 
		y, 
		pwindow->thread->preferences->global_plugin_dir, 
		"Global Plugin Path", 
		"Select the directory for plugins", 
		1));
	
	y += 35;
	add_subwindow(new BC_Title(x, y, "Look for personal plugins here", MEDIUMFONT, BLACK));
	y += 20;
	add_subwindow(lpathtext = new PluginLocalPathText(x, y, pwindow, pwindow->thread->preferences->local_plugin_dir));
	add_subwindow(lpath = new BrowseButton(mwindow,
		this,
		lpathtext, 
		215, 
		y, 
		pwindow->thread->preferences->local_plugin_dir, 
		"Personal Plugin Path", 
		"Select the directory for plugins", 
		1));


	return 0;
}




PluginGlobalPathText::PluginGlobalPathText(int x, int y, PreferencesWindow *pwindow, char *text)
 : BC_TextBox(x, y, 200, 1, text)
{ 
	this->pwindow = pwindow; 
}

PluginGlobalPathText::~PluginGlobalPathText() {}

int PluginGlobalPathText::handle_event()
{
	strcpy(pwindow->thread->preferences->global_plugin_dir, get_text());
}





PluginLocalPathText::PluginLocalPathText(int x, int y, PreferencesWindow *pwindow, char *text)
 : BC_TextBox(x, y, 200, 1, text)
{ 
	this->pwindow = pwindow; 
}

PluginLocalPathText::~PluginLocalPathText() {}

int PluginLocalPathText::handle_event()
{
	strcpy(pwindow->thread->preferences->local_plugin_dir, get_text());
}
