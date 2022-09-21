// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbar.h"
#include "bcresources.h"
#include "bctitle.h"
#include "deleteallindexes.h"
#include "edl.h"
#include "edlsession.h"
#include "guielements.h"
#include "language.h"
#include "interfaceprefs.h"
#include "mwindow.inc"
#include "preferences.h"
#include "preferencesthread.h"
#include "plugindb.h"
#include "pluginserver.h"
#include "theme.h"
#include "units.h"

#include <inttypes.h>

const struct selection_int ViewBehaviourSelection::viewbehaviour[] =
{
	{ N_("Drag all following edits"), MOVE_ALL_EDITS },
	{ N_("Drag only one edit"), MOVE_ONE_EDIT},
	{ N_("Drag source only"), MOVE_NO_EDITS },
	{ N_("No effect"), MOVE_EDITS_DISABLED },
	{ 0, 0 }
};

const struct selection_int InterfacePrefs::time_formats[] =
{
	{ N_("Use Hours:Minutes:Seconds.xxx"), TIME_HMS },
	{ N_("Use Hours:Minutes:Seconds:Frames"), TIME_HMSF },
	{ N_("Use Samples"), TIME_SAMPLES },
	{ N_("Use Frames"), TIME_FRAMES },
	{ N_("Use Seconds"), TIME_SECONDS },
	{ 0, 0 }
};

InterfacePrefs::InterfacePrefs(PreferencesWindow *pwindow)
 : PreferencesDialog(pwindow)
{
}

void InterfacePrefs::show()
{
	int y, x, value;
	BC_Resources *resources = BC_WindowBase::get_resources();
	BC_WindowBase *win;
	TextBox *ipathtext;
	RadialSelection *rsel;

	x = theme_global->preferencesoptions_x;
	y = theme_global->preferencesoptions_y;

	add_subwindow(new BC_Title(x, y, _("Time Format"),
		LARGEFONT, resources->text_default));

	y += get_text_height(LARGEFONT) + 5;

	add_subwindow(rsel = new RadialSelection(x, y, time_formats,
		&pwindow->thread->this_edlsession->time_format));
	rsel->show();
	y += rsel->get_h() + 10;

	add_subwindow(new CheckBox(x, y, _("Show tip of the day"),
		&pwindow->thread->preferences->use_tipwindow));
	y += 35;
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Index files"), LARGEFONT, resources->text_default));

	int ybix[3];
	int w, maxw;

	ybix[0] = y += 35;
	win = add_subwindow(new BC_Title(x, y + 5, _("Index files go here:"),
		MEDIUMFONT, resources->text_default));
	maxw = win->get_w();

	ybix[1] = y += 30;
	win = add_subwindow(new BC_Title(x, y + 5, _("Size of index file (kB):"),
		MEDIUMFONT, resources->text_default));
	if((w = win->get_w()) > maxw)
		maxw = w;

	ybix[2] = y += 30;
	win = add_subwindow(new BC_Title(x, y + 5, _("Number of index files to keep:"),
		MEDIUMFONT, resources->text_default));
	if((w = win->get_w()) > maxw)
		maxw = w;
	maxw += x + 5;

// Index path
	add_subwindow(ipathtext = new TextBox(maxw, ybix[0],
		pwindow->thread->preferences->index_directory, 240));

	add_subwindow(ipath = new BrowseButton(this, ipathtext,
		maxw + 5 + ipathtext->get_w(), ybix[0],
		pwindow->thread->preferences->index_directory,
		_("Index Path"), _("Select the directory for index files"), 1));

// Index file size
	add_subwindow(new ValueTextBox(maxw, ybix[1],
		&pwindow->thread->preferences->index_size, 100));

// Number of index files to keep
	win = add_subwindow(new ValueTextBox(maxw, ybix[2],
		&pwindow->thread->preferences->index_count, 100));
	add_subwindow(deleteall = new DeleteAllIndexes(pwindow,
		maxw + 10 + win->get_w(), ybix[2]));

	y += 35;
	add_subwindow(new BC_Bar(5, y, get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Editing"), LARGEFONT, resources->text_default));

	y += 35;
	add_subwindow(new CheckBox(x, y, _("Use thumbnails in resource window"),
		&pwindow->thread->preferences->use_thumbnails));

	y += 35;
	add_subwindow(new BC_Title(x, y, _("Dragging edit boundaries does what:")));
	y += 25;
	win = add_subwindow(new BC_Title(x, y, _("Button 1:")));
	maxw = win->get_w() + x + 10;

	ViewBehaviourSelection *selection;
	add_subwindow(selection = new ViewBehaviourSelection(maxw, y - 5, this,
		&pwindow->thread->this_edlsession->edit_handle_mode[0]));
	selection->update(pwindow->thread->this_edlsession->edit_handle_mode[0]);

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Button 2:")));
	add_subwindow(selection = new ViewBehaviourSelection(maxw, y - 5, this,
		&pwindow->thread->this_edlsession->edit_handle_mode[1]));
	selection->update(pwindow->thread->this_edlsession->edit_handle_mode[1]);

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Button 3:")));
	add_subwindow(selection = new ViewBehaviourSelection(maxw, y - 5, this,
		&pwindow->thread->this_edlsession->edit_handle_mode[2]));
	selection->update(pwindow->thread->this_edlsession->edit_handle_mode[2]);

	y += 30;
	int x1 = x;

	win = add_subwindow(new BC_Title(x, y + 5, _("Min DB for meter:")));
	x += win->get_w() + 5;

	win = add_subwindow(new ValueTextBox(x, y,
		&pwindow->thread->this_edlsession->min_meter_db, 50));
	x += win->get_w() + 10;

	win = add_subwindow(new BC_Title(x, y + 5, _("Max DB:")));
	x += win->get_w() + 5;

	add_subwindow(new ValueTextBox(x, y,
		&pwindow->thread->this_edlsession->max_meter_db, 50));

	x = x1;
	y += 30;
	ViewTheme *theme;
	add_subwindow(new BC_Title(x, y, _("Theme:")));
	x += 60;
	add_subwindow(theme = new ViewTheme(x, y, pwindow));
}


ViewTheme::ViewTheme(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, 200, pwindow->thread->preferences->theme, 1)
{
	ArrayList<PluginServer*> themes;

	plugindb.fill_plugindb(0,
		0,
		0,
		1,
		themes);

	this->pwindow = pwindow;

	for(int i = 0; i < themes.total; i++)
	{
		add_item(new ViewThemeItem(this, themes.values[i]->title));
	}
}


ViewThemeItem::ViewThemeItem(ViewTheme *popup, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
}

int ViewThemeItem::handle_event()
{
	popup->set_text(get_text());
	strcpy(popup->pwindow->thread->preferences->theme, get_text());
	popup->handle_event();
	return 1;
}


ViewBehaviourSelection::ViewBehaviourSelection(int x, int y,
	BC_WindowBase *window, int *value)
 : Selection(x, y, window, viewbehaviour, value, SELECTION_VARWIDTH)
{
	disable(1);
}

const char *ViewBehaviourSelection::name(int value)
{
	for(int i = 0; viewbehaviour[i].text; i++)
	{
		if(viewbehaviour[i].value == value)
			return _(viewbehaviour[i].text);
	}
	return _("Unknown");
}

void ViewBehaviourSelection::update(int value)
{
	BC_TextBox::update(name(value));
}
