#include "formattools.h"
//#include "loadmode.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "record.h"
#include "recordwindow.h"
#include "videodevice.inc"


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


#define WIDTH 410
#define HEIGHT 360


RecordWindow::RecordWindow(MWindow *mwindow, Record *record)
 : BC_Window(PROGRAM_NAME ": Record", 
 	mwindow->gui->get_root_w() / 2 - WIDTH / 2,
	mwindow->gui->get_root_h() / 2 - HEIGHT / 2,
 	WIDTH, 
	HEIGHT,
	(int)BC_INFINITY,
	(int)BC_INFINITY,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->record = record;
}

RecordWindow::~RecordWindow()
{
	delete format_tools;
//	delete loadmode;
}



int RecordWindow::create_objects()
{
//printf("RecordWindow::create_objects 1\n");
	add_subwindow(new BC_Title(5, 5, _("Select a file to record to:")));

//printf("RecordWindow::create_objects 1\n");
	int x = 5, y = 25;
	format_tools = new FormatTools(mwindow,
					this, 
					record->default_asset);
//printf("RecordWindow::create_objects 1\n");
	format_tools->create_objects(x, 
					y, 
					1, 
					1, 
					1, 
					1, 
					1, 
					1,
					record->fixed_compression,
					1,
					0,
					0);
//printf("RecordWindow::create_objects 1\n");

// Not the same as creating a new file at each label.
// Load mode is now located in the RecordGUI
	x = 10;
//	loadmode = new LoadMode(this, x, y, &record->load_mode, 1);
// 	loadmode->create_objects();

	add_subwindow(new BC_OKButton(this));
//printf("RecordWindow::create_objects 1\n");
	add_subwindow(new BC_CancelButton(this));
//printf("RecordWindow::create_objects 1\n");
	show_window();
//printf("RecordWindow::create_objects 2\n");
	return 0;
}







RecordToTracks::RecordToTracks(Record *record, int default_)
 : BC_CheckBox(200, 270, default_) { this->record = record; }
RecordToTracks::~RecordToTracks() 
{}
int RecordToTracks::handle_event()
{
	record->to_tracks = get_value();
}

