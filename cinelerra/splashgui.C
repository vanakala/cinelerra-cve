#include "bcdisplayinfo.h"
#include "mwindow.inc"
#include "splashgui.h"
#include "vframe.h"





SplashGUI::SplashGUI(VFrame *bg, int x, int y)
 : BC_Window(PROGRAM_NAME ": Loading",
		x,
		y,
		bg->get_w(),
		bg->get_h(),
		-1,
		-1,
		0,
		0,
		1)
{
	this->bg = bg;
}

SplashGUI::~SplashGUI()
{
}

void SplashGUI::create_objects()
{
	draw_vframe(bg, 0, 0);
// 	add_subwindow(operation = 
// 		new BC_Title(0, 
// 			get_h() - get_text_height(MEDIUMFONT),
// 			"Loading..."));
	flash();
	show_window();
}


