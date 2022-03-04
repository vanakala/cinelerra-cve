// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2006 Andraz Tori

#include "asset.h"
#include "bchash.h"
#include "bctitle.h"
#include "bcresources.h"
#include "bcrecentlist.h"
#include "browsebutton.h"
#include "confirmsave.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "exportedl.h"
#include "filesystem.h"
#include "language.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginserver.h"
#include "track.h"

#include <ctype.h>
#include <inttypes.h>
#include <string.h>

#define WIN_WIDTH 410
#define WIN_HEIGHT 400

ExportEDLAsset::ExportEDLAsset(MWindow *mwindow, EDL *edl)
{
	this->mwindow = mwindow;
	this->edl = edl;

	path[0] = 0;
	edl_type = EDLTYPE_CMX3600;
	track_number = -1;
}

void ExportEDLAsset::double_to_CMX3600(double seconds, double frame_rate, char *str)
{
	char tmp[20];
	Units::totext(tmp, 
			seconds, 
			TIME_HMSF, 
			0, // sample_rate ... unnecessary 
			frame_rate, 
			0);    // frames per foot
	if ((int)(seconds / 3600) <= 9)
	{
		str[0]='0';
		strcpy(str+1, tmp);
	}
	else
		strcpy(str, tmp);
}

void ExportEDLAsset::edit_to_timecodes(Edit *edit, char *sourceinpoint, char *sourceoutpoint, char *destinpoint, char *destoutpoint, char *reel_name)
{
	Asset *asset = edit->asset;
	Track *track = edit->track;
	double frame_rate = edlsession->frame_rate;

	double edit_sourcestart;
	double edit_sourceend;
	double edit_deststart;
	double edit_destend;

	if (asset)
	{
		// reelname should be 8 chars long

		strncpy(reel_name, "cin001", 9);
		reel_name[8] = 0;
		for (int i = strlen(reel_name); i<8; i++)
			reel_name[i] = ' ';

		edit_sourcestart = edit->get_source_pts();
		edit_sourceend = edit->end_pts();
	}
	else
	{
		strcpy(reel_name, "   BL   ");
		edit_sourcestart = 0;
		edit_sourceend = edit->length();
	}

	edit_deststart = edit->get_pts();
	edit_destend = edit->end_pts();

	double_to_CMX3600(edit_sourcestart, frame_rate, sourceinpoint);
	double_to_CMX3600(edit_sourceend, frame_rate, sourceoutpoint);
	double_to_CMX3600(edit_deststart, frame_rate, destinpoint);
	double_to_CMX3600(edit_destend, frame_rate, destoutpoint);
}

void ExportEDLAsset::export_it()
{
	FILE *fh;
	fh = fopen(path, "w+");

// We currently only support exporting one track at a time
// Find the track...
	int serial = 0;
	Track *track;
	for(track = edl->first_track();
		track;
		track = track->next)
	{
		if (serial == track_number) 
			break;
		serial ++;
	}

	int last_dissolve = 1;

	if (edl_type == EDLTYPE_CMX3600) 
	{

		// TODO: Find docs about exact header for CMX3600
		fprintf(fh, "TITLE: Cinproj   FORMAT: CMX 3600 4-Ch\n");

		int colnum = 1;

		for (Edit *edit = track->edits->first;
			edit;
			edit = edit->next)
		{
			char reel_name[BCTEXTLEN];
			char avselect[5];
			char edittype[5] = "C   ";
			char cutinfo[4] = "   ";
			char sourceinpoint[12];
			char sourceoutpoint[12];
			char destinpoint[12];
			char destoutpoint[12];
			if (track->data_type == TRACK_AUDIO)
				strcpy(avselect, "A   ");
			else
				strcpy(avselect, "V   ");

			if (edit->transition && !strcmp(edit->transition->plugin_server->title, "Dissolve"))
			{
				char last_sourceout[12];
				edit_to_timecodes(edit->previous, sourceinpoint, last_sourceout, destinpoint, destoutpoint, reel_name);
				edit_to_timecodes(edit, sourceinpoint, sourceoutpoint, destinpoint, destoutpoint, reel_name);

				if (last_dissolve)
				{
					fprintf(fh, "%03d %8s %s %4s %3s", colnum, reel_name, avselect, edittype, cutinfo);
					fprintf(fh, " %s %s", last_sourceout, last_sourceout);
					fprintf(fh, " %s %s", destinpoint, destinpoint);
					fprintf(fh,"\n");
				}
				else
				{
					colnum --;
				}
				edittype[0] = 'D';
				fprintf(fh, "%03d %8s %s %4s %03" PRId64, colnum, reel_name, avselect, edittype, track->to_units(edit->transition->get_length()));
				fprintf(fh, " %s %s", sourceinpoint, sourceoutpoint);
				fprintf(fh, " %s %s", destinpoint, destoutpoint);
				fprintf(fh,"\n");
				last_dissolve = 1;
			}
			else
			{
				edit_to_timecodes(edit, sourceinpoint, sourceoutpoint, destinpoint, destoutpoint, reel_name);
				fprintf(fh, "%03d %8s %s %4s %3s", colnum, reel_name, avselect, edittype, cutinfo);
				fprintf(fh, " %s %s", sourceinpoint, sourceoutpoint);
				fprintf(fh, " %s %s", destinpoint, destoutpoint);
				fprintf(fh,"\n");
				last_dissolve = 0;
			}
			colnum ++;
		}
	}

	fclose(fh);
}

void ExportEDLAsset::load_defaults()
{
	mwindow->defaults->get("EDLEXPORT_PATH", path);
	mwindow->defaults->get("EDLEXPORT_TYPE", edl_type);
	mwindow->defaults->get("EDLEXPORT_TRACKNUMBER", track_number);
}

void ExportEDLAsset::save_defaults()
{
	mwindow->defaults->update("EDLEXPORT_PATH", path);
	mwindow->defaults->update("EDLEXPORT_TYPE", edl_type);
	mwindow->defaults->update("EDLEXPORT_TRACKNUMBER", track_number);
}


ExportEDLItem::ExportEDLItem()
 : BC_MenuItem(_("Export EDL..."), "Shift+E", 'E')
{
	set_shift(1);
}

int ExportEDLItem::handle_event() 
{
	mwindow_global->exportedl->start_interactive();
	return 1;
}


ExportEDL::ExportEDL(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
}

void ExportEDL::start_interactive()
{
	if(!Thread::running())
	{
		Thread::start();
	}
}

void ExportEDL::run()
{
	int result = 0;
	int root_w, root_h;
	exportasset = new ExportEDLAsset(mwindow, master_edl);

	exportasset->load_defaults();

	BC_Resources::get_root_size(&root_w, &root_h);

// Get format from user
	result = 0;
	int filesok;

	do {
	// FIX
		filesok = 0;
		exportedl_window = new ExportEDLWindow(root_w / 2 - WIN_WIDTH / 2,
			root_h / 2 - WIN_HEIGHT / 2,
			mwindow, this, exportasset);
		result = exportedl_window->run_window();
		if(!result)
		{
			// add to recentlist only on OK
			// Fix "EDL"!
			exportedl_window->path_recent->add_item("EDLPATH", exportasset->path);
		}
		exportasset->track_number = exportedl_window->track_list->get_selection_number(0, 0);
		delete exportedl_window;
		exportedl_window = 0;
		if(!result)
		{
			ArrayList<char*> paths;

			paths.append(exportasset->path);
			filesok = ConfirmSave::test_files(mwindow, &paths);
		}
	} while(!result && filesok);
	mwindow->save_defaults();
	exportasset->save_defaults();

// FIX
	if(!result) exportasset->export_it();
	delete exportasset;
}

static const char *list_titles[] = 
{
	N_("No."),
	N_("Track name")
};

static int list_widths[] = 
{
	40,
	200
};

ExportEDLWindow::ExportEDLWindow(int x, int y, MWindow *mwindow,
	ExportEDL *exportedl, ExportEDLAsset *exportasset)
 : BC_Window(MWindow::create_title(N_("Export EDL")),
	x, y, WIN_WIDTH, WIN_HEIGHT,
	BC_INFINITY,
	BC_INFINITY,
	0,
	0,
	1)
{
	x = y = 5;

	this->mwindow = mwindow;
	this->exportasset = exportasset;

	set_icon(mwindow->get_window_icon());
	add_subwindow(new BC_Title(x, 
			y,
			_("Select a file to export to:")));
	y += 25;

	add_subwindow(path_textbox = new ExportEDLPathText(x, y, this));
	x += 300;
	path_recent = new BC_RecentList("EDLPATH", mwindow->defaults,
					path_textbox, 10, x, y, 300, 100);
	add_subwindow(path_recent);
// FIX
	path_recent->load_items("EDLPATH");

	x += 24;
	add_subwindow(path_button = new BrowseButton(
		mwindow,
		this,
		path_textbox, 
		x, 
		y - 4, 
		exportasset->path,
		_("Output to file"),
		_("Select a file to write to:"),
		0));

	y += 34;
	x = 5;
	add_subwindow(new BC_Title(x, y, _("Select track to be exported:")));
	y += 25;

	items_tracks[0].remove_all_objects();
	items_tracks[1].remove_all_objects();
	int serial = 0;
	if (exportasset->track_number == -1)
		exportasset->track_number = 0;
	for(Track *track = master_edl->first_track();
		track;
		track = track->next)
	{
		char tmp[16];
		sprintf(tmp, "%i\n", serial+1);

		BC_ListBoxItem *listitem = new BC_ListBoxItem(tmp);
		if (serial == exportasset->track_number)
			listitem->set_selected(1);
		items_tracks[0].append(listitem);
		items_tracks[1].append(new BC_ListBoxItem(track->title));
		serial ++;
	}

	add_subwindow(track_list = new ExportEDLWindowTrackList(this, x, y, 400, 200, items_tracks));

	y += 5 + track_list->get_h();
	add_subwindow(new BC_Title(x, y, _("Currently only CMX 3600 format is supported")));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
}


ExportEDLPathText::ExportEDLPathText(int x, int y, ExportEDLWindow *window)
 : BC_TextBox(x, y, 300, 1, window->exportasset->path) 
{
	this->window = window; 
}

int ExportEDLPathText::handle_event() 
{
	strcpy(window->exportasset->path, get_text());
	return 1;
}


ExportEDLWindowTrackList::ExportEDLWindowTrackList(ExportEDLWindow *window, 
	int x, 
	int y, 
	int w, 
	int h, 
	ArrayList<BC_ListBoxItem*> *track_list)
 : BC_ListBox(x, 
		y,
		w, 
		h, 
		track_list,
		0,
		list_titles,
		list_widths,
		2)
{
}
