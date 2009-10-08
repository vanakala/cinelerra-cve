
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "pluginprefs.h"
#include "preferences.h"
#include <string.h>


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


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

	add_subwindow(new BC_Title(x, y, _("Plugin Set"), LARGEFONT, BLACK));
	y += 35;
	add_subwindow(new BC_Title(x, y, _("Look for global plugins here"), MEDIUMFONT, BLACK));
	y += 20;
	add_subwindow(ipathtext = new PluginGlobalPathText(x, y, pwindow, pwindow->thread->preferences->global_plugin_dir));
	add_subwindow(ipath = new BrowseButton(mwindow,
		this,
		ipathtext, 
		215, 
		y, 
		pwindow->thread->preferences->global_plugin_dir, 
		_("Global Plugin Path"), 
		_("Select the directory for plugins"), 
		1));
	
	y += 35;
	add_subwindow(new BC_Title(x, y, _("Look for personal plugins here"), MEDIUMFONT, BLACK));
	y += 20;
	add_subwindow(lpathtext = new PluginLocalPathText(x, y, pwindow, pwindow->thread->preferences->local_plugin_dir));
	add_subwindow(lpath = new BrowseButton(mwindow,
		this,
		lpathtext, 
		215, 
		y, 
		pwindow->thread->preferences->local_plugin_dir, 
		_("Personal Plugin Path"), 
		_("Select the directory for plugins"), 
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
