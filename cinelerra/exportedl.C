
/*
 * CINELERRA
 * Copyright (C) 2006 Andraz Tori
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

#include "asset.h"
#include "bchash.h"
#include "condition.h"
#include "confirmsave.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "exportedl.h"
#include "tracks.h"
#include "transition.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



#include <ctype.h>
#include <string.h>

ExportEDLAsset::ExportEDLAsset(MWindow *mwindow, EDL *edl)
{
	this->mwindow = mwindow;
	this->edl = edl;
	
	path[0] = 0;
	edl_type = EDLTYPE_CMX3600;
	track_number = -1;
}

ExportEDLAsset::~ExportEDLAsset()
{
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
	} else
	{
		strcpy(str, tmp);
	}
	
//	str[8]='.';

	//sprintf(str, "%02d:%02d:%02d:%02d", hour, minute, second, hundredths);
}

int ExportEDLAsset::edit_to_timecodes(Edit *edit, char *sourceinpoint, char *sourceoutpoint, char *destinpoint, char *destoutpoint, char *reel_name)
{
	Asset *asset = edit->asset;
	Track *track = edit->track;
	double frame_rate = edit->track->edl->session->frame_rate;

	double edit_sourcestart;
	double edit_sourceend;
	double edit_deststart;
	double edit_destend;

	if (asset)
	{
		// reelname should be 8 chars long
		
		strncpy(reel_name, asset->reel_name, 9);
		if (strlen(asset->reel_name) > 8)
		{
			printf(_("Warning: chopping the reel name to eight characters!\n"));
		};
		reel_name[8] = 0;
		for (int i = strlen(reel_name); i<8; i++)
			reel_name[i] = ' ';
			
		edit_sourcestart = (double)asset->tcstart / asset->frame_rate
			+ track->from_units(edit->startsource);
		edit_sourceend = (double)asset->tcstart / asset->frame_rate
			+ track->from_units(edit->startsource + edit->length);

	} else
	{
		strcpy(reel_name, "   BL   ");
		edit_sourcestart = 0;
		edit_sourceend = track->from_units(edit->length);
	}
	
	edit_deststart = track->from_units(edit->startproject);
	edit_destend = track->from_units(edit->startproject + edit->length);

	double_to_CMX3600(edit_sourcestart, frame_rate, sourceinpoint);
	double_to_CMX3600(edit_sourceend, frame_rate, sourceoutpoint);
	double_to_CMX3600(edit_deststart, frame_rate, destinpoint);
	double_to_CMX3600(edit_destend, frame_rate, destoutpoint);
	
	return 0;
}


int ExportEDLAsset::export_it()
{
	FILE *fh;
	fh = fopen(path, "w+");

// We currently only support exporting one track at a time
// Find the track...
	int serial = 0;
	Track *track;
	for(track = edl->tracks->first;
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
			
			//if (edit->transition)
			//	printf("title: %s, length: %i\n", edit->transition->title, edit->transition->length);
			if (edit->transition && !strcmp(edit->transition->title, "Dissolve"))
			{
				char last_sourceout[12];
				edit_to_timecodes(edit->previous, sourceinpoint, last_sourceout, destinpoint, destoutpoint, reel_name);
				edit_to_timecodes(edit, sourceinpoint, sourceoutpoint, destinpoint, destoutpoint, reel_name);

				if (last_dissolve)
				{
					fprintf(fh, "%03d %8s %s %4s %03s", colnum, reel_name, avselect, edittype, cutinfo);
					fprintf(fh, " %s %s", last_sourceout, last_sourceout);
					fprintf(fh, " %s %s", destinpoint, destinpoint);
					fprintf(fh,"\n");		
				} else
				{
					colnum --;
				}
				edittype[0] = 'D';
				fprintf(fh, "%03d %8s %s %4s %03d", colnum, reel_name, avselect, edittype, edit->transition->length);
				fprintf(fh, " %s %s", sourceinpoint, sourceoutpoint);
				fprintf(fh, " %s %s", destinpoint, destoutpoint);
				fprintf(fh,"\n");
				last_dissolve = 1;		
			} else
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



int ExportEDLAsset::load_defaults()
{
	mwindow->defaults->get("EDLEXPORT_PATH", path);
	mwindow->defaults->get("EDLEXPORT_TYPE", edl_type);
	mwindow->defaults->get("EDLEXPORT_TRACKNUMBER", track_number);
	//load_mode = mwindow->defaults->get("RENDER_LOADMODE", LOAD_NEW_TRACKS);


	return 0;
}

int ExportEDLAsset::save_defaults()
{
	mwindow->defaults->update("EDLEXPORT_PATH", path);
	mwindow->defaults->update("EDLEXPORT_TYPE", edl_type);
	mwindow->defaults->update("EDLEXPORT_TRACKNUMBER", track_number);
	return 0;
}




ExportEDLItem::ExportEDLItem(MWindow *mwindow)
 : BC_MenuItem(_("Export EDL..."), "Shift+E", 'E')
{
	this->mwindow = mwindow;
	set_shift(1);
}

int ExportEDLItem::handle_event() 
{
	mwindow->exportedl->start_interactive();
	return 1;
}





ExportEDL::ExportEDL(MWindow *mwindow)
 : Thread(0, 0, 0)
{
	this->mwindow = mwindow;
//	package_lock = new Mutex("ExportEDL::package_lock");
//	counter_lock = new Mutex("ExportEDL::counter_lock");
//	completion = new Condition(0, "ExportEDL::completion");
//	progress_timer = new Timer;
}

ExportEDL::~ExportEDL()
{
//	delete package_lock;
//	delete counter_lock;
//	delete completion;
///	if(preferences) delete preferences;
//	delete progress_timer;
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
	exportasset = new ExportEDLAsset(mwindow, mwindow->edl);
	
	exportasset->load_defaults();

// Get format from user
		result = 0;
		int filesok;

		do {
		// FIX
			filesok = 0;
			exportedl_window = new ExportEDLWindow(mwindow, this, exportasset);
			exportedl_window->create_objects();
			result = exportedl_window->run_window();
			if (! result) {
				// add to recentlist only on OK
				// Fix "EDL"!
				exportedl_window->path_recent->add_item("EDLPATH", exportasset->path);
			}
			exportasset->track_number = exportedl_window->track_list->get_selection_number(0, 0);

			delete exportedl_window;
			exportedl_window = 0;
			if (!result)
			{
				ArrayList<char*> paths;
			
				paths.append(exportasset->path);
				filesok = ConfirmSave::test_files(mwindow, &paths);
			}
			
		} while (!result && filesok);
	mwindow->save_defaults();
	exportasset->save_defaults();

// FIX
	if(!result) exportasset->export_it();


	delete exportasset;

}








#define WIDTH 410
#define HEIGHT 400

static char *list_titles[] = 
{
	N_("No."),
	N_("Track name")
};


static int list_widths[] = 
{
	40,
	200
};
	
ExportEDLWindow::ExportEDLWindow(MWindow *mwindow, ExportEDL *exportedl, ExportEDLAsset *exportasset)
 : BC_Window(PROGRAM_NAME ": Export EDL", 
 	mwindow->gui->get_root_w(0, 1) / 2 - WIDTH / 2,
	mwindow->gui->get_root_h(1) / 2 - HEIGHT / 2,
 	WIDTH, 
	HEIGHT,
	(int)BC_INFINITY,
	(int)BC_INFINITY,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->exportasset = exportasset;
}

ExportEDLWindow::~ExportEDLWindow()
{
//	delete format_tools;
//	delete loadmode;
}



int ExportEDLWindow::create_objects()
{
	int x = 5, y = 5;
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
	for(Track *track = mwindow->edl->tracks->first;
		track;
		track = track->next)
	{
		
		char tmp[10];
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
	return 0;
}


ExportEDLPathText::ExportEDLPathText(int x, int y, ExportEDLWindow *window)
 : BC_TextBox(x, y, 300, 1, window->exportasset->path) 
{
	this->window = window; 
}
ExportEDLPathText::~ExportEDLPathText() 
{
}
int ExportEDLPathText::handle_event() 
{
	strcpy(window->exportasset->path, get_text());
//	window->handle_event();
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
		LISTBOX_TEXT, 
		track_list,
		list_titles,
		list_widths,
		2)
{ 
	this->window = window; 
}

int ExportEDLWindowTrackList::handle_event() 
{
//	window->exportasset->track_number = get_selection_number(0, 0);
//	printf("aaaaa %i\n", window->exportasset->track_number );
//	window->set_done(0); 
}



