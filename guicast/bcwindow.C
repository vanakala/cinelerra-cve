#include "bcwindow.h"




BC_Window::BC_Window(char *title, 
				int x,
				int y,
				int w, 
				int h, 
				int minw, 
				int minh, 
				int allow_resize,
				int private_color, 
				int hide,
				int bg_color,
				char *display_name,
				int group_it)
 : BC_WindowBase()
{
	create_window(0,
				title, 
				x,
				y,
				w, 
				h, 
				(minw < 0) ? w : minw, 
				(minh < 0) ? h : minh, 
				allow_resize,
				private_color, 
				hide,
				bg_color,
				display_name,
				MAIN_WINDOW,
				0,
				group_it);
}


BC_Window::~BC_Window()
{
}

