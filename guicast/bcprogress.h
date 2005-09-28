#ifndef BCPROGRESS_H
#define BCPROGRESS_H

#include "bcsubwindow.h"

class BC_ProgressBar : public BC_SubWindow
{
public:
	BC_ProgressBar(int x, int y, int w, int64_t length, int do_text = 1);
	~BC_ProgressBar();

	int initialize();
	int reposition_window(int x, int y, int w = -1, int h = -1);
	void set_do_text(int value);

	int update(int64_t position);
	int update_length(int64_t length);
	int set_images();

private:
	int draw(int force = 0);

	int64_t length, position;
	int pixel;
	int do_text;
	BC_Pixmap *images[2];
};

#endif
