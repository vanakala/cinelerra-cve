#ifndef BCPROGRESS_H
#define BCPROGRESS_H

#include "bcsubwindow.h"

class BC_ProgressBar : public BC_SubWindow
{
public:
	BC_ProgressBar(int x, int y, int w, long length, int do_text = 1);
	~BC_ProgressBar();

	int initialize();
	int reposition_window(int x, int y, int w = -1, int h = -1);
	void set_do_text(int value);

	int update(long position);
	int update_length(long length);
	int set_images();

private:
	int draw(int force = 0);

	long length, position;
	int pixel;
	int do_text;
	BC_Pixmap *images[2];
};

#endif
