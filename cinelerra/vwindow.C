#include "asset.h"
#include "assets.h"
#include "clipedit.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "tracks.h"
#include "transportque.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"


VWindow::VWindow(MWindow *mwindow) : Thread()
{
	this->mwindow = mwindow;
	edl = 0;
	asset = 0;
}


VWindow::~VWindow()
{
//printf("VWindow::~VWindow 1\n");
	delete playback_engine;
//printf("VWindow::~VWindow 1\n");
	delete playback_cursor;
	delete_edl();
	delete clip_edit;
//printf("VWindow::~VWindow 2\n");
}

void VWindow::delete_edl()
{
//printf("VWindow::delete_edl 1\n");
	if(mwindow->edl->vwindow_edl)
	{
		delete mwindow->edl->vwindow_edl;
		mwindow->edl->vwindow_edl = 0;
	}

	if(asset) delete asset;
	asset = 0;
	edl_shared = 0;
	edl = 0;
}


void VWindow::load_defaults()
{
}

int VWindow::create_objects()
{
//printf("VWindow::create_objects 1\n");
	gui = new VWindowGUI(mwindow, this);
//printf("VWindow::create_objects 1\n");
	gui->create_objects();
//printf("VWindow::create_objects 1\n");

	playback_engine = new VPlayback(mwindow, this, gui->canvas);
//printf("VWindow::create_objects 1\n");

// Start command loop
	playback_engine->create_objects();
//printf("VWindow::create_objects 1\n");
	gui->transport->set_engine(playback_engine);
//printf("VWindow::create_objects 1\n");
	playback_cursor = new VTracking(mwindow, this);
//printf("VWindow::create_objects 1\n");
	playback_cursor->create_objects();
//printf("VWindow::create_objects 2\n");

	clip_edit = new ClipEdit(mwindow, 0, this);
	return 0;
}

void VWindow::run()
{
	gui->run_window();
}

EDL* VWindow::get_edl()
{
//printf("VWindow::get_edl 1 %p\n", edl);
	return edl;
}

Asset* VWindow::get_asset()
{
	return this->asset;
}

void VWindow::change_source()
{
//printf("VWindow::change_source() 1 %p\n", mwindow->edl->vwindow_edl);
	if(mwindow->edl->vwindow_edl)
	{
		this->edl = mwindow->edl->vwindow_edl;
		gui->change_source(edl, "");
		update_position(CHANGE_ALL, 1, 1);
	}
	else
	{
		if(asset) delete asset;
		asset = 0;
		edl_shared = 0;
		edl = 0;
	}
}

void VWindow::change_source(Asset *asset)
{
//printf("VWindow::change_source 1\n");
// 	if(asset && this->asset &&
// 		asset->id == this->asset->id &&
// 		asset == this->asset) return;

//printf("VWindow::change_source(Asset *asset) 1\n");

	char title[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(title, asset->path);
//printf("VWindow::change_source 1\n");

	delete_edl();
//printf("VWindow::change_source 1\n");

// Generate EDL off of main EDL for cutting
	this->asset = new Asset;
	*this->asset = *asset;
	edl = mwindow->edl->vwindow_edl = new EDL(mwindow->edl);
	edl_shared = 0;
	edl->create_objects();
	mwindow->asset_to_edl(edl, asset);
//printf("VWindow::change_source 1 %d %d\n", edl->local_session->loop_playback, mwindow->edl->local_session->loop_playback);
//edl->dump();

// Update GUI
	gui->change_source(edl, title);
	update_position(CHANGE_ALL, 1, 1);


// Update master session
	strcpy(mwindow->edl->session->vwindow_folder, MEDIA_FOLDER);
	mwindow->edl->session->vwindow_source = 0;
	int i = 0;
	for(Asset *current = mwindow->edl->assets->first; 
		current;
		current = NEXT)
	{
		if(this->asset->equivalent(*current, 0, 0))
		{
			mwindow->edl->session->vwindow_source = i;
			break;
		}
		i++;
	}

//printf("VWindow::change_source 2\n");
}

void VWindow::change_source(EDL *edl)
{
//printf("VWindow::change_source(EDL *edl) 1\n");
//printf("VWindow::change_source %p\n", edl);
// EDLs are identical
	if(edl && mwindow->edl->vwindow_edl && 
		edl->id == mwindow->edl->vwindow_edl->id) return;

	delete_edl();

	if(edl)
	{
		this->asset = 0;
		this->edl = edl;
		edl_shared = 1;
//printf("VWindow::change_source 1\n");
//edl->dump();
//printf("VWindow::change_source 2\n");

// Update GUI
		gui->change_source(edl, edl->local_session->clip_title);
		update_position(CHANGE_ALL, 1, 1);

// Update master session
		strcpy(mwindow->edl->session->vwindow_folder, CLIP_FOLDER);
		mwindow->edl->session->vwindow_source = 
			mwindow->edl->clips.number_of(edl);
	}
	else
		gui->change_source(edl, _("Viewer"));
}


void VWindow::remove_source()
{
	delete_edl();
	gui->change_source(0, _("Viewer"));
}

void VWindow::change_source(char *folder, int item)
{
//printf("VWindow::change_source(char *folder, int item) 1\n");
	int result = 0;
// Search EDLs
	if(!strcasecmp(folder, CLIP_FOLDER))
	{
		if(item < mwindow->edl->clips.total)
		{
			change_source(mwindow->edl->clips.values[item]);
			result = 1;
		}
	}
	else
// Search media
	if(!strcasecmp(folder, MEDIA_FOLDER))
	{
		if(item < mwindow->edl->assets->total())
		{
			change_source(mwindow->edl->assets->get_item_number(item));
			result = 1;
		}
	}
	else
// Search extra clip folders
	{
	}
	
	if(!result)
	{
		remove_source();
	}
}




void VWindow::goto_start()
{
	if(edl)
	{
		edl->local_session->selectionstart = 
			edl->local_session->selectionend = 0;
		update_position(CHANGE_NONE, 
			0, 
			1);
	}
}

void VWindow::goto_end()
{
	if(edl)
	{
		edl->local_session->selectionstart = 
			edl->local_session->selectionend = 
			edl->tracks->total_length();
		update_position(CHANGE_NONE, 
			0, 
			1);
	}
}

void VWindow::update(int do_timebar)
{
	if(do_timebar)
		gui->timebar->update();
}

void VWindow::update_position(int change_type, 
	int use_slider, 
	int update_slider)
{
	EDL *edl = get_edl();
	if(edl)
	{
		if(use_slider) 
		{
			edl->local_session->selectionstart = 
				edl->local_session->selectionend = 
				gui->slider->get_value();
		}

		if(update_slider)
		{
			gui->slider->set_position();
		}

		playback_engine->que->send_command(CURRENT_FRAME, 
			change_type,
			edl,
			1);

		gui->clock->update(edl->local_session->selectionstart);
	}
}

void VWindow::set_inpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->set_inpoint(edl->local_session->selectionstart);
		gui->timebar->update();
	}
}

void VWindow::set_outpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->set_outpoint(edl->local_session->selectionstart);
		gui->timebar->update();
	}
}

void VWindow::clear_inpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->local_session->in_point = -1;
		gui->timebar->update();
	}
}

void VWindow::clear_outpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->local_session->out_point = -1;
		gui->timebar->update();
	}
}

void VWindow::copy()
{
	EDL *edl = get_edl();
	if(edl)
	{
		double start = edl->local_session->get_selectionstart();
		double end = edl->local_session->get_selectionend();
		FileXML file;
		edl->copy(start,
			end,
			0,
			0,
			0,
			&file,
			mwindow->plugindb,
			"",
			1);
		mwindow->gui->lock_window();
		mwindow->gui->get_clipboard()->to_clipboard(file.string,
			strlen(file.string),
			SECONDARY_SELECTION);
		mwindow->gui->unlock_window();
	}
}

void VWindow::splice_selection()
{
}

void VWindow::overwrite_selection()
{
}



