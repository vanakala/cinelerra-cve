#include "assets.h"
#include "edl.h"
#include "filexml.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include <string.h>

MainUndo::MainUndo(MWindow *mwindow)
{ 
	this->mwindow = mwindow;
	undo_before_updated = 0;
}

MainUndo::~MainUndo()
{
}

void MainUndo::update_undo_before(char *description, uint32_t load_flags)
{
	if(!undo_before_updated)
	{
		FileXML file;
		mwindow->session->changes_made = 1;
		mwindow->edl->save_xml(mwindow->plugindb, 
			&file, 
			"",
			0,
			0);
		file.terminate_string();

		current_entry = undo_stack.push();
		current_entry->load_flags = load_flags;
		current_entry->set_data_before(file.string);
		current_entry->set_description(description);

// the after update is always without a description
		mwindow->gui->lock_window();
		mwindow->gui->mainmenu->undo->update_caption(description);
		mwindow->gui->mainmenu->redo->update_caption("");
		mwindow->gui->unlock_window();

		undo_before_updated = 1;
	}
}

void MainUndo::update_undo_after()
{
	if(undo_before_updated)
	{
		FileXML file;
		mwindow->edl->save_xml(mwindow->plugindb, 
			&file, 
			"",
			0,
			0);
		file.terminate_string();
		current_entry->set_data_after(file.string);
		undo_before_updated = 0;
	}
}






int MainUndo::undo()
{
	if(undo_stack.current)
	{
		current_entry = undo_stack.current;
		if(current_entry->description && mwindow->gui) 
			mwindow->gui->mainmenu->redo->update_caption(current_entry->description);
		
		FileXML file;
		
		file.read_from_string(current_entry->data_before);
		load_from_undo(&file, current_entry->load_flags);
		
		undo_stack.pull();    // move current back
		if(mwindow->gui)
		{
			current_entry = undo_stack.current;
			if(current_entry)
				mwindow->gui->mainmenu->undo->update_caption(current_entry->description);
			else
				mwindow->gui->mainmenu->undo->update_caption("");
		}
	}
	return 0;
}

int MainUndo::redo()
{
	current_entry = undo_stack.pull_next();
	
	if(current_entry)
	{
		FileXML file;
		file.read_from_string(current_entry->data_after);
		load_from_undo(&file, current_entry->load_flags);

		if(mwindow->gui)
		{
			mwindow->gui->mainmenu->undo->update_caption(current_entry->description);
			
			if(current_entry->next)
				mwindow->gui->mainmenu->redo->update_caption(current_entry->next->description);
			else
				mwindow->gui->mainmenu->redo->update_caption("");
		}
	}
	return 0;
}


// Here the master EDL loads 
int MainUndo::load_from_undo(FileXML *file, uint32_t load_flags)
{
	mwindow->edl->load_xml(mwindow->plugindb, file, load_flags);
	for(Asset *asset = mwindow->edl->assets->first;
		asset;
		asset = asset->next)
	{
		mwindow->mainindexes->add_next_asset(asset);
	}
	mwindow->mainindexes->start_build();
	return 0;
}




