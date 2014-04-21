
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

#include "deleteallindexes.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "interfaceprefs.h"
#include "theme.h"

#define MOVE_ALL_EDITS_TITLE "Drag all following edits"
#define MOVE_ONE_EDIT_TITLE "Drag only one edit"
#define MOVE_NO_EDITS_TITLE "Drag source only"
#define MOVE_EDITS_DISABLED_TITLE "No effect"

InterfacePrefs::InterfacePrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
}

void InterfacePrefs::show()
{
	int y, x, value;
	char string[1024];
	BC_Resources *resources = BC_WindowBase::get_resources();

	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;

	add_subwindow(new BC_Title(x, 
		y, 
		_("Time Format"), 
		LARGEFONT, 
		resources->text_default));

	y += get_text_height(LARGEFONT) + 5;
	add_subwindow(hms = new TimeFormatHMS(this,
		pwindow->thread->edl->session->time_format == TIME_HMS, 
		x, 
		y));
	y += 20;
	add_subwindow(hmsf = new TimeFormatHMSF(this,
		pwindow->thread->edl->session->time_format == TIME_HMSF, 
		x, 
		y));
	y += 20;
	add_subwindow(samples = new TimeFormatSamples(this,
		pwindow->thread->edl->session->time_format == TIME_SAMPLES, 
		x, 
		y));
	y += 20;
	add_subwindow(hex = new TimeFormatHex(this,
		pwindow->thread->edl->session->time_format == TIME_SAMPLES_HEX, 
		x, 
		y));
	y += 20;
	add_subwindow(frames = new TimeFormatFrames(this,
		pwindow->thread->edl->session->time_format == TIME_FRAMES, 
		x, 
		y));
	y += 20;
	add_subwindow(feet = new TimeFormatFeet(this,
		pwindow->thread->edl->session->time_format == TIME_FEET_FRAMES, 
		x, 
		y));

	add_subwindow(new BC_Title(260, y, _("frames per foot")));
	sprintf(string, "%0.2f", pwindow->thread->edl->session->frames_per_foot);
	add_subwindow(new TimeFormatFeetSetting(pwindow, 
		x + 155, 
		y - 5, 
		string));
	y += 20;
	add_subwindow(seconds = new TimeFormatSeconds(this,
		pwindow->thread->edl->session->time_format == TIME_SECONDS, 
		x, 
		y));

	y += 35;
	add_subwindow(new UseTipWindow(pwindow, x, y));

	y += 35;
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Index files"), LARGEFONT, resources->text_default));

	y += 35;
	add_subwindow(new BC_Title(x, 
		y + 5, 
		_("Index files go here:"), MEDIUMFONT, resources->text_default));
	add_subwindow(ipathtext = new IndexPathText(x + 230, 
		y, 
		pwindow, 
		pwindow->thread->preferences->index_directory));
	add_subwindow(ipath = new BrowseButton(mwindow,
		this,
		ipathtext, 
		x + 230 + ipathtext->get_w(), 
		y, 
		pwindow->thread->preferences->index_directory,
		_("Index Path"), 
		_("Select the directory for index files"),
		1));

	y += 30;
	add_subwindow(new BC_Title(x, 
		y + 5, 
		_("Size of index file:"), 
		MEDIUMFONT, 
		resources->text_default));
	sprintf(string, "%lld", pwindow->thread->preferences->index_size);
	add_subwindow(isize = new IndexSize(x + 230, y, pwindow, string));
	y += 30;
	add_subwindow(new BC_Title(x, y + 5, _("Number of index files to keep:"), MEDIUMFONT, resources->text_default));
	sprintf(string, "%d", pwindow->thread->preferences->index_count);
	add_subwindow(icount = new IndexCount(x + 230, y, pwindow, string));
	add_subwindow(deleteall = new DeleteAllIndexes(mwindow, pwindow, 350, y));

	y += 35;
	add_subwindow(new BC_Bar(5, y, get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Editing"), LARGEFONT, resources->text_default));

	y += 35;
	add_subwindow(thumbnails = new ViewThumbnails(x, y, pwindow));

	y += 35;
	add_subwindow(new BC_Title(x, y, _("Dragging edit boundaries does what:")));
	y += 25;
	add_subwindow(new BC_Title(x, y, _("Button 1:")));

	ViewBehaviourText *text;
	add_subwindow(text = new ViewBehaviourText(80, 
		y - 5, 
		behavior_to_text(pwindow->thread->edl->session->edit_handle_mode[0]), 
			pwindow, 
			&(pwindow->thread->edl->session->edit_handle_mode[0])));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Button 2:")));
	add_subwindow(text = new ViewBehaviourText(80, 
		y - 5, 
		behavior_to_text(pwindow->thread->edl->session->edit_handle_mode[1]), 
			pwindow, 
			&(pwindow->thread->edl->session->edit_handle_mode[1])));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Button 3:")));
	add_subwindow(text = new ViewBehaviourText(80, 
		y - 5, 
		behavior_to_text(pwindow->thread->edl->session->edit_handle_mode[2]), 
			pwindow, 
			&(pwindow->thread->edl->session->edit_handle_mode[2])));

	y += 35;
	int x1 = x;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y + 5, _("Min DB for meter:")));
	x += title->get_w() + 10;
	sprintf(string, "%d", pwindow->thread->edl->session->min_meter_db);
	add_subwindow(min_db = new MeterMinDB(pwindow, string, x, y));

	x += min_db->get_w() + 10;
	add_subwindow(title = new BC_Title(x, y + 5, _("Max DB:")));
	x += title->get_w() + 10;
	sprintf(string, "%d", pwindow->thread->edl->session->max_meter_db);
	add_subwindow(max_db = new MeterMaxDB(pwindow, string, x, y));

	x = x1;
	y += 25;
	ViewTheme *theme;
	add_subwindow(new BC_Title(x, y, _("Theme:")));
	x += 60;
	add_subwindow(theme = new ViewTheme(x, y, pwindow));
}

const char* InterfacePrefs::behavior_to_text(int mode)
{
	switch(mode)
	{
	case MOVE_ALL_EDITS:
		return _(MOVE_ALL_EDITS_TITLE);
	case MOVE_ONE_EDIT:
		return _(MOVE_ONE_EDIT_TITLE);
	case MOVE_NO_EDITS:
		return _(MOVE_NO_EDITS_TITLE);
	case MOVE_EDITS_DISABLED:
		return _(MOVE_EDITS_DISABLED_TITLE);
	default:
		return "";
	}
}

void InterfacePrefs::update(int new_value)
{
	pwindow->thread->redraw_times = 1;
	pwindow->thread->edl->session->time_format = new_value;
	hms->update(new_value == TIME_HMS);
	hmsf->update(new_value == TIME_HMSF);
	samples->update(new_value == TIME_SAMPLES);
	hex->update(new_value == TIME_SAMPLES_HEX);
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
	delete hex;
	delete feet;
	delete min_db;
	delete max_db;
	delete thumbnails;
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
	long result;

	result = atol(get_text());
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


TimeFormatHex::TimeFormatHex(InterfacePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, _("Use Hex Samples"))
{
	this->tfwindow = tfwindow;
}

int TimeFormatHex::handle_event()
{
	tfwindow->update(TIME_SAMPLES_HEX);
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

TimeFormatFeetSetting::TimeFormatFeetSetting(PreferencesWindow *pwindow, int x, int y, const char *string)
 : BC_TextBox(x, y, 90, 1, string)
{
	this->pwindow = pwindow;
}

int TimeFormatFeetSetting::handle_event()
{
	pwindow->thread->edl->session->frames_per_foot = atof(get_text());
	if(pwindow->thread->edl->session->frames_per_foot < 1)
		pwindow->thread->edl->session->frames_per_foot = 1;
	return 1;
}

ViewBehaviourText::ViewBehaviourText(int x, 
	int y, 
	const char *text, 
	PreferencesWindow *pwindow, 
	int *output)
 : BC_PopupMenu(x, y, 200, text)
{
	this->output = output;
	add_item(new ViewBehaviourItem(this, _(MOVE_ALL_EDITS_TITLE), MOVE_ALL_EDITS));
	add_item(new ViewBehaviourItem(this, _(MOVE_ONE_EDIT_TITLE), MOVE_ONE_EDIT));
	add_item(new ViewBehaviourItem(this, _(MOVE_NO_EDITS_TITLE), MOVE_NO_EDITS));
	add_item(new ViewBehaviourItem(this, _(MOVE_EDITS_DISABLED_TITLE), MOVE_EDITS_DISABLED));
}


ViewBehaviourItem::ViewBehaviourItem(ViewBehaviourText *popup, const char *text, int behaviour)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->behaviour = behaviour;
}

int ViewBehaviourItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = behaviour;
}


MeterMinDB::MeterMinDB(PreferencesWindow *pwindow, const char *text, int x, int y)
 : BC_TextBox(x, y, 50, 1, text)
{ 
	this->pwindow = pwindow; 
}

int MeterMinDB::handle_event()
{ 
	pwindow->thread->redraw_meters = 1;
	pwindow->thread->edl->session->min_meter_db = atol(get_text()); 
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
	pwindow->thread->edl->session->max_meter_db = atol(get_text()); 
	return 1;
}


ViewTheme::ViewTheme(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, 200, pwindow->thread->preferences->theme, 1)
{
	ArrayList<PluginServer*> themes;

	pwindow->mwindow->create_plugindb(0, 
		0, 
		0, 
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


ViewThumbnails::ViewThumbnails(int x, 
	int y, 
	PreferencesWindow *pwindow)
 : BC_CheckBox(x, 
	y,
	pwindow->thread->preferences->use_thumbnails, _("Use thumbnails in resource window"))
{
	this->pwindow = pwindow;
}

int ViewThumbnails::handle_event()
{
	pwindow->thread->preferences->use_thumbnails = get_value();
	return 1;
}


UseTipWindow::UseTipWindow(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, 
	y,
	pwindow->thread->preferences->use_tipwindow, 
	_("Show tip of the day"))
{
	this->pwindow = pwindow;
}

int UseTipWindow::handle_event()
{
	pwindow->thread->preferences->use_tipwindow = get_value();
	return 1;
}
