#include "mwindow.h"
#include "mwindowgui.h"
#include "recconfirmdelete.h"

RecConfirmDelete::RecConfirmDelete(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Confirm", 
 	mwindow->gui->get_abs_cursor_x(),
	mwindow->gui->get_abs_cursor_y(),
 	320, 100)
{
}

RecConfirmDelete::~RecConfirmDelete()
{
}

int RecConfirmDelete::create_objects(char *string)
{
	char string2[256];
	int x = 10, y = 10;
	sprintf(string2, "Delete this file and %s?", string);
	add_subwindow(new BC_Title(x, y, string2));
	y += 30;
	add_subwindow(new BC_OKButton(x, y));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(x, y));
	return 0;
}











