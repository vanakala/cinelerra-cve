#ifndef BCPIXMAPSW_H
#define BCPIXMAPSW_H

#include "bcpixmapsw.h"
#include "vframe.inc"
#include "bcsubwindow.h"


class BC_PixmapSW : public BC_SubWindow 
{
public:
	BC_PixmapSW(int x, int y, BC_Pixmap *thepixmap);
	virtual ~BC_PixmapSW();

	int initialize();
	virtual int handle_event() { return 0; };
	virtual char* get_caption() { return ""; };

	int reposition_widget(int x, int y);

private:
	int draw();
	BC_Pixmap *thepixmap;
};


#endif
