#include "errorbox.h"

ErrorBox::ErrorBox(char *title, int x, int y, int w, int h)
 : BC_Window(title, x, y, w, h, w, h, 0)
{
}

ErrorBox::~ErrorBox()
{
}

int ErrorBox::create_objects(char *text)
{
	int x = 10, y = 10;
	BC_Title *title;

	add_subwindow(new BC_Title(get_w() / 2, y, text, MEDIUMFONT, BLACK, 1));
	x = get_w() / 2 - 30;
	y = get_h() - 50;
	add_tool(new BC_OKButton(x, y));
	return 0;
}
