#ifndef BCDISPLAYINFO_H
#define BCDISPLAYINFO_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

class BC_DisplayInfo
{
public:
	BC_DisplayInfo(char *display_name = "", int show_error = 1);
	~BC_DisplayInfo();
	
	friend class BC_WindowBase;

	int get_root_w();
	int get_root_h();
	int get_abs_cursor_x();
	int get_abs_cursor_y();
	static void parse_geometry(char *geom, int *x, int *y, int *width, int *height);
// Get window border size created by window manager
	int get_top_border();
	int get_left_border();
	int get_right_border();
	int get_bottom_border();
	void test_window(int &x_out, int &y_out, int &x_out2, int &y_out2, int x_in, int y_in);


private:
	void init_borders();
	void init_window(char *display_name, int show_error);
	Display* display;
	Window rootwin;
	Visual *vis;
	int screen;
	static int top_border;
	static int left_border;
	static int bottom_border;
	static int right_border;
	static int auto_reposition_x;
	static int auto_reposition_y;
	int default_depth;
	char *display_name;
};

#endif
