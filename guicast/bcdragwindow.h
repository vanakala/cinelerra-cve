#ifndef BCDRAGWINDOW_H
#define BCDRAGWINDOW_H

#include "bcpixmap.inc"
#include "bcpopup.h"

class BC_DragWindow : public BC_Popup
{
public:
	BC_DragWindow(BC_WindowBase *parent_window, BC_Pixmap *pixmap, int icon_x, int icon_y);
	BC_DragWindow(BC_WindowBase *parent_window, VFrame *frame, int icon_x, int icon_y);
	~BC_DragWindow();

	int cursor_motion_event();
	int drag_failure_event();
	int get_offset_x();
	int get_offset_y();
// Disable failure animation
	void set_animation(int value);
	BC_Pixmap *prepare_frame(VFrame *frame, BC_WindowBase *parent_window);
private:
	static int get_init_x(BC_WindowBase *parent_window, int icon_x);
	static int get_init_y(BC_WindowBase *parent_window, int icon_y);

	int init_x, init_y;
	int end_x, end_y;
	int icon_offset_x, icon_offset_y;
	int do_animation;
	VFrame *temp_frame;
	BC_Pixmap *my_pixmap;
};

#endif
