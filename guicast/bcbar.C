#include "bcbar.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "vframe.h"




BC_Bar::BC_Bar(int x, int y, int w, VFrame *data)
 : BC_SubWindow(x, y, w, 0, -1)
{
	this->data = data;
	image = 0;
}

BC_Bar::~BC_Bar()
{
	delete image;
}

int BC_Bar::initialize()
{
	if(data)
		set_image(data);
	else
		set_image(get_resources()->bar_data);

// Create the subwindow
	BC_SubWindow::initialize();

	draw();
	return 0;
}

void BC_Bar::set_image(VFrame *data)
{
	delete image;
	image = new BC_Pixmap(parent_window, data, PIXMAP_ALPHA);
	h = image->get_h();
}

int BC_Bar::reposition_window(int x, int y, int w)
{
	BC_WindowBase::reposition_window(x, y, w, -1);
	draw();
	return 0;
}

int BC_Bar::resize_event(int w, int h)
{
	reposition_window(x, y, get_w());
	return 1;
}


void BC_Bar::draw()
{
	draw_top_background(parent_window, 0, 0,w, h);
	draw_3segmenth(0, 0, w, 0, w, image);
	flash();
}
