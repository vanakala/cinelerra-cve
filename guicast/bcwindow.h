#ifndef BCWINDOW_H
#define BCWINDOW_H

#include "bcwindowbase.h"

class BC_Window : public BC_WindowBase
{
public:
	BC_Window(char *title, 
				int x,
				int y,
				int w, 
				int h, 
				int minw = -1, 
				int minh = -1, 
				int allow_resize = 1,
				int private_color = 0, 
				int hide = 0,
				int bg_color = -1,
				char *display_name = "",
				int group_it = 1);
	virtual ~BC_Window();

private:
};


#endif
