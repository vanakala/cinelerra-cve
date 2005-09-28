#include "confirmsave.h"
#include "defaults.h"
#include "edl.h"
#include "errorbox.h"
#include "file.h"
#include "filexml.h"
#include "fileformat.h"
#include "indexfile.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "savefile.h"
#include "mainsession.h"

#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)









SaveBackup::SaveBackup(MWindow *mwindow)
 : BC_MenuItem(_("Save backup"))
{
	this->mwindow = mwindow;
}
int SaveBackup::handle_event()
{
	mwindow->save_backup();
	mwindow->gui->show_message(_("Saved backup."));
	return 1;
}











Save::Save(MWindow *mwindow) : BC_MenuItem(_("Save"), "s", 's')
{ 
	this->mwindow = mwindow; 
	quit_now = 0; 
}

int Save::create_objects(SaveAs *saveas)
{
	this->saveas = saveas;
	return 0;
}

int Save::handle_event()
{
	if(mwindow->session->filename[0] == 0) 
	{
		saveas->start();
	}
	else
	{
// save it
// TODO: Move this into mwindow.
		FileXML file;
		mwindow->edl->save_xml(mwindow->plugindb, 
			&file, 
			mwindow->session->filename,
			0,
			0);
		file.terminate_string();

		if(file.write_to_file(mwindow->session->filename))
		{
			char string2[256];
			sprintf(string2, _("Couldn't open %s"), mwindow->session->filename);
			ErrorBox error(PROGRAM_NAME ": Error",
				mwindow->gui->get_abs_cursor_x(1),
				mwindow->gui->get_abs_cursor_y(1));
			error.create_objects(string2);
			error.run_window();
			return 1;		
		}
		else
		{
			char string[BCTEXTLEN];
			sprintf(string, _("\"%s\" %dC written"), mwindow->session->filename, strlen(file.string));
			mwindow->gui->show_message(string);
		}
		mwindow->session->changes_made = 0;
		if(saveas->quit_now) mwindow->gui->set_done(0);
	}
	return 1;
}

int Save::save_before_quit()
{
	saveas->quit_now = 1;
	handle_event();
	return 0;
}

SaveAs::SaveAs(MWindow *mwindow)
 : BC_MenuItem(_("Save as..."), ""), Thread()
{ 
	this->mwindow = mwindow; 
	quit_now = 0;
}

int SaveAs::set_mainmenu(MainMenu *mmenu)
{
	this->mmenu = mmenu;
	return 0;
}

int SaveAs::handle_event() 
{ 
	quit_now = 0;
	start();
	return 1;
}

void SaveAs::run()
{
// ======================================= get path from user
	int result;
//printf("SaveAs::run 1\n");
	char directory[1024], filename[1024];
	sprintf(directory, "~");
	mwindow->defaults->get("DIRECTORY", directory);

// Loop if file exists
	do{
		SaveFileWindow *window;

		window = new SaveFileWindow(mwindow, directory);
		window->create_objects();
		result = window->run_window();
		mwindow->defaults->update("DIRECTORY", window->get_submitted_path());
		strcpy(filename, window->get_submitted_path());
		delete window;

// Extend the filename with .xml
		if(strlen(filename) < 4 || 
			strcasecmp(&filename[strlen(filename) - 4], ".xml"))
		{
			strcat(filename, ".xml");
		}

// ======================================= try to save it
		if(filename[0] == 0) return;              // no filename given
		if(result == 1) return;          // user cancelled
		result = ConfirmSave::test_file(mwindow, filename);
	}while(result);        // file exists so repeat

//printf("SaveAs::run 6 %s\n", filename);




// save it
	FileXML file;
	mwindow->set_filename(filename);      // update the project name
	mwindow->edl->save_xml(mwindow->plugindb, 
		&file, 
		filename,
		0,
		0);
	file.terminate_string();

	if(file.write_to_file(filename))
	{
		char string2[256];
		mwindow->set_filename("");      // update the project name
		sprintf(string2, _("Couldn't open %s."), filename);
		ErrorBox error(PROGRAM_NAME ": Error",
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
		error.create_objects(string2);
		error.run_window();
		return;		
	}
	else
	{
		char string[BCTEXTLEN];
		sprintf(string, _("\"%s\" %dC written"), filename, strlen(file.string));
		mwindow->gui->lock_window();
		mwindow->gui->show_message(string);
		mwindow->gui->unlock_window();
	}


	mwindow->session->changes_made = 0;
	mmenu->add_load(filename);
	if(quit_now) mwindow->gui->set_done(0);
	return;
}








SaveFileWindow::SaveFileWindow(MWindow *mwindow, char *init_directory)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(1),
 	mwindow->gui->get_abs_cursor_y(1) - BC_WindowBase::get_resources()->filebox_h / 2,
 	init_directory, 
	PROGRAM_NAME ": Save", 
	_("Enter a filename to save as"))
{ 
	this->mwindow = mwindow; 
}

SaveFileWindow::~SaveFileWindow() {}

