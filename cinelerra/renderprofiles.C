
/*
 * CINELERRA
 * Copyright (C) 2007 Andraz Tori
 * Copyright (C) 2017 Einar Rünkaru
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

#include "bcsignals.h"
#include "bctitle.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "renderprofiles.h"
#include "mwindow.h"
#include "theme.h"
#include "bchash.h"
#include "string.h"
#include "render.h"
#include "asset.h"
#include "mainerror.h"
#include "language.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define LISTWIDTH 200
#define MAX_PROFILES 16


RenderProfile::RenderProfile(MWindow *mwindow,
	RenderWindow *rwindow, 
	int x, 
	int y)
{
	DIR *dir;
	struct dirent *entry;
	struct stat stb;
	char string[BCTEXTLEN];

	this->mwindow = mwindow;
	this->rwindow = rwindow;
	this->x = x;
	this->y = y;

	rwindow->add_subwindow(new BC_Title(x, 
		y, 
		_("RenderProfile:")));

	int old_y = y;
	rwindow->add_subwindow(title = new BC_Title(x, y, _("Render profile:")));
	y += 25;
	rwindow->add_subwindow(textbox = new BC_TextBox(x, 
		y, 
		LISTWIDTH, 
		1, 
		""));
	x += textbox->get_w();
	rwindow->add_subwindow(listbox = new RenderProfileListBox(rwindow, this, x, y));

	y = old_y;
	x += listbox->get_w() + 10;
	rwindow->add_subwindow(saveprofile = new SaveRenderProfileButton(this, 
		x, 
		y));
	y += 25;
	rwindow->add_subwindow(deleteprofile = new DeleteRenderProfileButton(this, 
		x, 
		y));

	edlsession->configuration_path(RENDERCONFIG_DIR, string);
	if(dir = opendir(string))
	{
		char *pe = &string[strlen(string)];
		*pe++ = '/';

		while(entry = readdir(dir))
		{
			if(entry->d_name[0] == '.')
				continue;
			strcpy(pe, entry->d_name);
			if(!stat(string, &stb) && S_ISDIR(stb.st_mode))
				merge_profile(entry->d_name);
		}
		closedir(dir);
	}
	else
		create_profile(RENDERCONFIG_DFLT);

	strcpy(string, RENDERCONFIG_DFLT);
	mwindow_global->defaults->get("RENDERPROFILE", string);
	select_profile(string);
}

RenderProfile::~RenderProfile()
{
	char string[BCTEXTLEN];

	for(int i = 0; i < profiles.total; i++)
		delete profiles.values[i];
	// Remove old defaults
	for (int i = 1; i < MAX_PROFILES; i++)
	{
		sprintf(string, "RENDER_%i_PROFILE_NAME", i);
		mwindow->defaults->delete_key(string);
		sprintf(string, "RENDER_%i_STRATEGY", i);
		mwindow->defaults->delete_key(string);
		sprintf(string, "RENDER_%i_LOADMODE", i);
		mwindow->defaults->delete_key(string);
		sprintf(string, "RENDER_%i_RANGE_TYPE", i);
		mwindow->defaults->delete_key(string);
	}
}

int RenderProfile::create_profile(const char *profile)
{
	char *p;
	char *config_path = rwindow->asset->renderprofile_path;

	edlsession->configuration_path(RENDERCONFIG_DIR, config_path);
	if(chk_profile_dir(config_path))
		return 1;

	p = &config_path[strlen(config_path)];
	*p++ = '/';

	strncpy(p, profile, BCTEXTLEN - (p - config_path) - 1);
	config_path[BCTEXTLEN - 1] = 0;

	if(chk_profile_dir(config_path))
		return 1;

	merge_profile(profile);
	select_profile(profile);
	return 0;
}

int RenderProfile::chk_profile_dir(const char *dirname)
{
	DIR *dir;

	if(dir = opendir(dirname))
		closedir(dir);
	else if(mkdir(dirname, 0755))
		return 1;
	return 0;
}

void RenderProfile::remove_profiledir(const char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	char str[BCTEXTLEN];
	char *p;

	strcpy(str, dirname);
	p = &str[strlen(str)];

	if(dir = opendir(str))
	{
		*p++ = '/';
		while(entry = readdir(dir))
		{
			if(entry->d_name[0] == '.')
				continue;
			strcpy(p, entry->d_name);

			if(unlink(str))
				perror("unlink");
		}
		closedir(dir);
		*--p = 0;

		if(rmdir(str))
			perror("rmdir");
	}
}

int RenderProfile::select_profile(const char *profile)
{
	DIR *dir;
	char *p;
	char *config_path = rwindow->asset->renderprofile_path;

	edlsession->configuration_path(RENDERCONFIG_DIR, config_path);

	p = &config_path[strlen(config_path)];
	*p++ = '/';
	strcpy(p, profile);

	if(dir = opendir(config_path))
	{
		closedir(dir);
		textbox->update(profile);
		mwindow->defaults->update("RENDERPROFILE", profile);
		rwindow->load_profile();
		return 0;
	}
	else if(strcmp(profile, RENDERCONFIG_DFLT))
	{
		select_profile(RENDERCONFIG_DFLT);
		return 0;
	}
	return 1;
}

void RenderProfile::merge_profile(const char *profile)
{
	if(!profile || !profile[0])
		return;

	for(int i = 0; i < profiles.total; i++)
	{
		if(strcmp(profile, profiles.values[i]->get_text()) < 0)
		{
			profiles.insert(new BC_ListBoxItem(profile), i);
			return;
		}
	}
	profiles.append(new BC_ListBoxItem(profile));
}

int RenderProfile::calculate_h(BC_WindowBase *gui)
{
	return BC_TextBox::calculate_h(gui, MEDIUMFONT, 1, 1);
}

int RenderProfile::get_h()
{
	int result = 0;
	result = MAX(result, title->get_h());
	result = MAX(result, textbox->get_h());
	return result;
}

int RenderProfile::get_x()
{
	return x;
}

int RenderProfile::get_y()
{
	return y;
}

void RenderProfile::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	title->reposition_window(x, y);
	y += 20;
	textbox->reposition_window(x, y);
	x += textbox->get_w();
	listbox->reposition_window(x, 
		y, 
		LISTWIDTH);
}


RenderProfileListBox::RenderProfileListBox(BC_WindowBase *window, 
	RenderProfile *renderprofile, 
	int x, 
	int y)
 : BC_ListBox(x,
	y,
	LISTWIDTH,
	150,
	(ArrayList<BC_ListBoxItem *>*)&renderprofile->profiles,
	LISTBOX_POPUP)
{
	this->window = window;
	this->renderprofile = renderprofile;
}

int RenderProfileListBox::handle_event()
{
	if(get_selection(0, 0))
		renderprofile->select_profile(get_selection(0, 0)->get_text());
	return 1;
}


SaveRenderProfileButton::SaveRenderProfileButton(RenderProfile *profile, int x, int y)
 : BC_GenericButton(x, y, _("Save profile"))
{
	this->profile = profile;
}

int SaveRenderProfileButton::handle_event()
{
	char *profile_name = profile->textbox->get_text();

	if (strlen(profile_name) == 0)     // Don't save when name not defined
		return 1;

	if(profile->create_profile(profile_name))
	{
		if(profile->select_profile(profile_name))
		{
			errorbox("Failed to create new profile '%s'", profile_name);
			profile->select_profile(RENDERCONFIG_DFLT);
		}
		return 1;
	}
	return 1;
}


DeleteRenderProfileButton::DeleteRenderProfileButton(RenderProfile *profile, int x, int y)
 : BC_GenericButton(x, y, _("Delete profile"))
{
	this->profile = profile;
}

int DeleteRenderProfileButton::handle_event()
{
	DIR *dir;
	char *profile_name = profile->textbox->get_text();
	char *p;
	char str[BCTEXTLEN];

	// Do not delete default profile
	if(strcmp(profile_name, RENDERCONFIG_DFLT) == 0)
		return 0;
	edlsession->configuration_path(RENDERCONFIG_DIR, str);
	p = &str[strlen(str)];
	*p++ = '/';
	strcpy(p, profile_name);

	RenderProfile::remove_profiledir(str);

	for(int i = 0; i < profile->profiles.total; i++)
	{
		if(strcmp(profile_name, profile->profiles.values[i]->get_text())== 0)
		{
			profile->profiles.remove_object(profile->profiles.values[i]);
			if(i >= profile->profiles.total)
				i = 0;
			profile->select_profile(profile->profiles.values[i]->get_text());
			break;
		}
	}
	return 1;
}
