#include "asset.h"
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

void MainUndo::push_undo_item(UndoStackItem *item)
{
   undo_stack.push(item);
   undo_stack.prune();
   mwindow->gui->lock_window("MainUndo::update_undo_before");
   mwindow->gui->mainmenu->undo->update_caption(item->description);
   mwindow->gui->mainmenu->redo->update_caption("");
   mwindow->gui->unlock_window();
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
      current_entry->set_mwindow(mwindow);

// the after update is always without a description
		mwindow->gui->lock_window("MainUndo::update_undo_before");
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
//printf("MainUndo::update_undo_after 1\n");
		mwindow->edl->save_xml(mwindow->plugindb, 
			&file, 
			"",
			0,
			0);
//printf("MainUndo::update_undo_after 1\n");
		file.terminate_string();
//printf("MainUndo::update_undo_after 1\n");
		current_entry->set_data_after(file.string);
		undo_stack.prune();
//printf("MainUndo::update_undo_after 10\n");
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

      current_entry->undo();

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
      current_entry->redo();

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
