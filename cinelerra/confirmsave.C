#include "confirmsave.h"
#include "mwindow.h"
#include "mwindowgui.h"

ConfirmSave::ConfirmSave(MWindow *mwindow)
{
	this->mwindow = mwindow;
}

ConfirmSave::~ConfirmSave()
{
}

int ConfirmSave::test_file(char *filename)
{
	FILE *in;
	if(in = fopen(filename, "rb"))
	{
		fclose(in);
		ConfirmSaveWindow cwindow(mwindow, filename);
		cwindow.create_objects();
		int result = cwindow.run_window();
		return result;
	}
	return 0;
}






ConfirmSaveWindow::ConfirmSaveWindow(MWindow *mwindow, char *filename)
 : BC_Window(PROGRAM_NAME ": File Exists", 
 		mwindow->gui->get_abs_cursor_x() - 140, 
		mwindow->gui->get_abs_cursor_y() - 80, 
		375, 
		160)
{
	this->filename = filename;
}

ConfirmSaveWindow::~ConfirmSaveWindow()
{
}

int ConfirmSaveWindow::create_objects()
{
	char string[1024];
	int x = 10, y = 10;
	sprintf(string, "Overwrite %s?", filename);
	add_subwindow(new BC_Title(5, 5, string));
	y += 30;
	add_subwindow(new BC_OKButton(this));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(this));
	return 0;
}
