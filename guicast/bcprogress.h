// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCPROGRESS_H
#define BCPROGRESS_H

#include "bcsubwindow.h"

class BC_ProgressBar : public BC_SubWindow
{
public:
	BC_ProgressBar(int x, int y, int w, double length, int do_text = 1);
	~BC_ProgressBar();

	void initialize();
	void reposition_window(int x, int y, int w = -1, int h = -1);
	void set_do_text(int value);

	void update(double position);
	void update_length(double length);
	void set_images();

private:
	void draw(int force = 0);

	double length;
	double position;
	int pixel;
	int do_text;
	BC_Pixmap *image_up;
	BC_Pixmap *image_hi;
};

#endif
