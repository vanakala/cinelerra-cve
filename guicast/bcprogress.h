#ifndef BCPROGRESS_H
#define BCPROGRESS_H

#include "bcsubwindow.h"

class BC_ProgressBar : public BC_SubWindow
{
public:
	BC_ProgressBar(int x, int y, int w, long length);

	int initialize();

	int update(long position);
	int update_length(long length);
	int set_images();

private:
	int draw(int force = 0);

	long length, position;
	int pixel;
	BC_Pixmap *images[2];
};

#endif
