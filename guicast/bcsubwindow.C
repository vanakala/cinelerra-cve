#include "bcsubwindow.h"



BC_SubWindow::BC_SubWindow(int x, int y, int w, int h, int bg_color)
 : BC_WindowBase()
{
	this->x = x; 
	this->y = y; 
	this->w = w; 
	this->h = h;
	this->bg_color = bg_color;
//printf("BC_SubWindow::BC_SubWindow 1\n");
}

BC_SubWindow::~BC_SubWindow()
{
}

int BC_SubWindow::initialize()
{
	create_window(parent_window, 
			"", 
			x, 
			y, 
			w, 
			h, 
			0, 
			0, 
			0, 
			0, 
			0, 
			bg_color, 
			NULL, 
			SUB_WINDOW, 
			0);
	return 0;
}






BC_SubWindowList::BC_SubWindowList()
 : ArrayList<BC_WindowBase*>()
{
}

BC_SubWindowList::~BC_SubWindowList()
{
}
