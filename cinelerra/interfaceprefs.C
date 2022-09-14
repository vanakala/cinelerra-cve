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

InterfacePrefs::InterfacePrefs(PreferencesWindow *pwindow)
 : PreferencesDialog(pwindow)
{
}

void InterfacePrefs::show()
{
	int y, x, value;
	BC_Resources *resources = BC_WindowBase::get_resources();
	BC_WindowBase *win;
	char string[BCTEXTLEN];

	x = theme_global->preferencesoptions_x;
	y = theme_global->preferencesoptions_y;

	add_subwindow(new BC_Title(x, y, _("Time Format"),
		LARGEFONT, resources->text_default));

	y += get_text_height(LARGEFONT) + 5;
	add_subwindow(hms = new TimeFormatHMS(this,
		pwindow->thread->this_edlsession->time_format == TIME_HMS,
		x, 
		y));
	y += 20;
	add_subwindow(hmsf = new TimeFormatHMSF(this,
		pwindow->thread->this_edlsession->time_format == TIME_HMSF,
		x, 
		y));
	y += 20;
	add_subwindow(samples = new TimeFormatSamples(this,
		pwindow->thread->this_edlsession->time_format == TIME_SAMPLES,
		x, 
		y));
	y += 20;
	add_subwindow(frames = new TimeFormatFrames(this,
		pwindow->thread->this_edlsession->time_format == TIME_FRAMES,
		x, 
		y));
	y += 20;
	add_subwindow(feet = new TimeFormatFeet(this,
		pwindow->thread->this_edlsession->time_format == TIME_FEET_FRAMES,
		x, 
		y));
	win = add_subwindow(new DblValueTextBox(x + feet->get_w() + 5, y - 2,
		&pwindow->thread->this_edlsession->frames_per_foot,
		GUIELEM_VAL_W, 2));
	add_subwindow(new BC_Title(x + feet->get_w() + win->get_w() + 10,
		y, _("frames per foot")));
	y += 20;
	add_subwindow(seconds = new TimeFormatSeconds(this,
		pwindow->thread->this_edlsession->time_format == TIME_SECONDS,
		x, 
		y));

	y += 35;
	add_subwindow(new CheckBox(x, y, _("Show tip of the day"),
		&pwindow->thread->preferences->use_tipwindow));
	y += 35;
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Index files"), LARGEFONT, resources->text_default));

	int ybix[3];
	int w, maxw;

	ybix[0] = y += 35;
	win = add_subwindow(new BC_Title(x,
		y + 5, 
		_("Index files go here:"), MEDIUMFONT, resources->text_default));
	maxw = win->get_w();

	ybix[1] = y += 30;
	win = add_subwindow(new BC_Title(x, 
		y + 5, 
		_("Size of index file:"), 
		MEDIUMFONT, 
		resources->text_default));
	if((w = win->get_w()) > maxw)
		maxw = w;

	ybix[2] = y += 30;
	win = add_subwindow(new BC_Title(x, y + 5, _("Number of index files to keep:"), MEDIUMFONT, resources->text_default));
	if((w = win->get_w()) > maxw)
		maxw = w;
	maxw += x + 5;

// Index path
	add_subwindow(ipathtext = new IndexPathText(maxw,
		ybix[0],
		pwindow, 
		pwindow->thread->preferences->index_directory));
	add_subwindow(ipath = new BrowseButton(this,
		ipathtext, 
		maxw + 5 + ipathtext->get_w(),
		y, 
		pwindow->thread->preferences->index_directory,
		_("Index Path"), 
		_("Select the directory for index files"),
		1));

// Index file size
	sprintf(string, "%d", pwindow->thread->preferences->index_size);
	add_subwindow(isize = new IndexSize(maxw, ybix[1], pwindow, string));

// Number of index files to keep
	sprintf(string, "%d", pwindow->thread->preferences->index_count);
	add_subwindow(icount = new IndexCount(maxw, ybix[2], pwindow, string));
	add_subwindow(deleteall = new DeleteAllIndexes(pwindow,
		maxw + 10 + icount->get_w(), ybix[2]));

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

	y += 35;
	int x1 = x;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y + 5, _("Min DB for meter:")));
	x += title->get_w() + 10;
	sprintf(string, "%d", pwindow->thread->this_edlsession->min_meter_db);
	add_subwindow(min_db = new MeterMinDB(pwindow, string, x, y));

	x += min_db->get_w() + 10;
	add_subwindow(title = new BC_Title(x, y + 5, _("Max DB:")));
	x += title->get_w() + 10;
	sprintf(string, "%d", pwindow->thread->this_edlsession->max_meter_db);
	add_subwindow(max_db = new MeterMaxDB(pwindow, string, x, y));

	x = x1;
	y += 25;
	ViewTheme *theme;
	add_subwindow(new BC_Title(x, y, _("Theme:")));
	x += 60;
	add_subwindow(theme = new ViewTheme(x, y, pwindow));
}

void InterfacePrefs::update(int new_value)
{
	pwindow->thread->redraw_times = 1;
	pwindow->thread->this_edlsession->time_format = new_value;
	hms->update(new_value == TIME_HMS);
	hmsf->update(new_value == TIME_HMSF);
	samples->update(new_value == TIME_SAMPLES);
	frames->update(new_value == TIME_FRAMES);
	feet->update(new_value == TIME_FEET_FRAMES);
	seconds->update(new_value == TIME_SECONDS);
}

InterfacePrefs::~InterfacePrefs()
{
	delete hms;
	delete hmsf;
	delete samples;
	delete frames;
	delete feet;
	delete min_db;
	delete max_db;
}


IndexPathText::IndexPathText(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	const char *text)
 : BC_TextBox(x, y, 240, 1, text)
{
	this->pwindow = pwindow; 
}

int IndexPathText::handle_event()
{
	strcpy(pwindow->thread->preferences->index_directory, get_text());
	return 1;
}


IndexSize::IndexSize(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	const char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
}

int IndexSize::handle_event()
{
	int result;

	result = atoi(get_text());
	if(result < 64000) result = 64000;
	pwindow->thread->preferences->index_size = result;
	return 1;
}


IndexCount::IndexCount(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	const char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
}

int IndexCount::handle_event()
{
	long result;

	result = atol(get_text());
	if(result < 1) result = 1;
	pwindow->thread->preferences->index_count = result;
	return 1;
}


TimeFormatHMS::TimeFormatHMS(InterfacePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Use Hours:Minutes:Seconds.xxx"))
{
	this->tfwindow = tfwindow;
}

int TimeFormatHMS::handle_event()
{
	tfwindow->update(TIME_HMS);
	return 1;
}


TimeFormatHMSF::TimeFormatHMSF(InterfacePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Use Hours:Minutes:Seconds:Frames"))
{
	this->tfwindow = tfwindow;
}

int TimeFormatHMSF::handle_event()
{
	tfwindow->update(TIME_HMSF);
	return 1;
}


TimeFormatSamples::TimeFormatSamples(InterfacePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Use Samples"))
{
	this->tfwindow = tfwindow;
}

int TimeFormatSamples::handle_event()
{
	tfwindow->update(TIME_SAMPLES);
	return 1;
}


TimeFormatFrames::TimeFormatFrames(InterfacePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Use Frames"))
{
	this->tfwindow = tfwindow;
}

int TimeFormatFrames::handle_event()
{
	tfwindow->update(TIME_FRAMES);
	return 1;
}


TimeFormatSeconds::TimeFormatSeconds(InterfacePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Use Seconds"))
{ 
	this->tfwindow = tfwindow;
}

int TimeFormatSeconds::handle_event()
{
	tfwindow->update(TIME_SECONDS);
	return 1;
}

TimeFormatFeet::TimeFormatFeet(InterfacePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Use Feet-frames"))
{
	this->tfwindow = tfwindow;
}

int TimeFormatFeet::handle_event()
{
	tfwindow->update(TIME_FEET_FRAMES);
	return 1;
}


MeterMinDB::MeterMinDB(PreferencesWindow *pwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 50, 1, text)
{ 
	this->pwindow = pwindow; 
}

int MeterMinDB::handle_event()
{ 
	pwindow->thread->redraw_meters = 1;
	pwindow->thread->this_edlsession->min_meter_db = atol(get_text());
	return 1;
}


MeterMaxDB::MeterMaxDB(PreferencesWindow *pwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 50, 1, text)
{ 
	this->pwindow = pwindow; 
}

int MeterMaxDB::handle_event()
{ 
	pwindow->thread->redraw_meters = 1;
	pwindow->thread->this_edlsession->max_meter_db = atol(get_text());
	return 1;
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
