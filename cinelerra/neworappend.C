#include "assets.h"
#include "confirmsave.h"
#include "file.h"
#include "filesystem.h"
#include "keys.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "neworappend.h"
#include "theme.h"

NewOrAppend::NewOrAppend(MWindow *mwindow)
{
	this->mwindow = mwindow;
}

NewOrAppend::~NewOrAppend()
{
}

// give the user three options if the file exists
int NewOrAppend::test_file(Asset *asset, FileXML *script)
{
	FILE *file;
	int result = 0;
	if(file = fopen(asset->path, "r"))
	{
		fclose(file);
		if(!script)
		{
// Don't ask user if running a script.
			ConfirmSaveWindow window(mwindow, asset->path);
			window.create_objects();
			result = window.run_window();
		}
		else
		{
			result = 0;
		}
		return result;
	}
	else
	{
		return 0;
	}
	
	return 0;
}






NewOrAppendWindow::NewOrAppendWindow(MWindow *mwindow, Asset *asset, int confidence)
 : BC_Window(PROGRAM_NAME ": Overwrite", 
 		mwindow->gui->get_abs_cursor_x(),
		mwindow->gui->get_abs_cursor_y(),
		375, 
		160)
{
	this->mwindow = mwindow;
	this->asset = asset;
	this->confidence = confidence;
}

NewOrAppendWindow::~NewOrAppendWindow()
{
	delete ok;
	delete cancel;
	delete append;
}

int NewOrAppendWindow::create_objects()
{
	char string[1024], filename[1024];
	FileSystem fs;
	fs.extract_name(filename, asset->path);
	
	sprintf(string, "%s exists!", filename);
	add_subwindow(new BC_Title(5, 5, string));
	if(confidence == 1)
	sprintf(string, "But is in the same format as your new file.");
	else
	sprintf(string, "But might be in the same format as your new file.");
	add_subwindow(new BC_Title(5, 25, string));
	add_subwindow(ok = new NewOrAppendNewButton(this));
	add_subwindow(append = new NewOrAppendAppendButton(this));
	add_subwindow(cancel = new NewOrAppendCancelButton(this));
	return 0;
}

NewOrAppendNewButton::NewOrAppendNewButton(NewOrAppendWindow *nwindow)
 : BC_Button(30, 45, 0)
{
	this->nwindow = nwindow;
}

int NewOrAppendNewButton::handle_event()
{
	nwindow->set_done(0);
}

int NewOrAppendNewButton::keypress_event()
{
	if(nwindow->get_keypress() == 13) { handle_event(); return 1; }
	return 0;
}

NewOrAppendAppendButton::NewOrAppendAppendButton(NewOrAppendWindow *nwindow)
 : BC_Button(30, 80, 0)
{
	this->nwindow = nwindow;
}

int NewOrAppendAppendButton::handle_event()
{
	nwindow->set_done(2);
}

int NewOrAppendAppendButton::keypress_event()
{
	if(nwindow->get_keypress() == 13) { handle_event(); return 1; }
	return 0;
}

NewOrAppendCancelButton::NewOrAppendCancelButton(NewOrAppendWindow *nwindow)
 : BC_Button(30, 115, 0)
{
	this->nwindow = nwindow;
}

int NewOrAppendCancelButton::handle_event()
{
	nwindow->set_done(1);
}

int NewOrAppendCancelButton::keypress_event()
{
	if(nwindow->get_keypress() == ESC) { handle_event(); return 1; }
	return 0;
}

