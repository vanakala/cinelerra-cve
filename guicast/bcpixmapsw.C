#include "bcpixmap.h"
#include "bcpixmapsw.h"
#include "bcresources.h"
#include "colors.h"
#include "keys.h"
#include "units.h"
#include "vframe.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

BC_PixmapSW::BC_PixmapSW(int x, int y, BC_Pixmap *thepixmap)
 : BC_SubWindow(x, y, -1, -1, -1)
{
	this->thepixmap = thepixmap;
}

BC_PixmapSW::~BC_PixmapSW()
{
}

int BC_PixmapSW::initialize()
{
	w = thepixmap->get_w();
	h = thepixmap->get_h();

	BC_SubWindow::initialize();
	draw();
	return 0;
}

int BC_PixmapSW::reposition_widget(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw();
	return 0;
}

int BC_PixmapSW::draw()
{
	draw_top_background(parent_window, 0, 0, get_w(), get_h());
	draw_pixmap(thepixmap);
	flash();
	return 0;
}

