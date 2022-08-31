// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCTITLE_H
#define BCTITLE_H

#include "bcsubwindow.h"
#include "colors.h"
#include "fonts.h"

class BC_Title : public BC_SubWindow
{
public:
	BC_Title(int x, 
		int y, 
		const char *text = 0,
		int font = MEDIUMFONT, 
		int color = -1, 
		int centered = 0,
		int fixed_w = 0);

	BC_Title(int x,
		int y,
		const wchar_t *text,
		int font = MEDIUMFONT,
		int color = -1,
		int centered = 0,
		int fixed_w = 0);

	BC_Title(int x,
		int y,
		int value,
		int font = MEDIUMFONT,
		int color = -1,
		int centered = 0,
		int fixed_w = 0);
	virtual ~BC_Title();

	void initialize();
	static int calculate_w(BC_WindowBase *gui, const char *text, int font = MEDIUMFONT);
	static int calculate_h(BC_WindowBase *gui, const char *text, int font = MEDIUMFONT);
	void resize(int w, int h);
	void reposition(int x, int y);
	void set_color(int color);
	void update(const char *text);
	void update(const wchar_t *text);
	void update(double value, int precision = 4);
	void update(int value);
	char* get_text();

private:
	void draw();
	static void get_size(BC_WindowBase *gui, int font, const char *text, 
		int fixed_w, int &w, int &h);
	static void get_size(BC_WindowBase *gui, int font, const wchar_t *text,
		int fixed_w, int &w, int &h);

	char text[BCTEXTLEN];
	wchar_t *wide_title;

	int color;
	int font;
	int centered;
// Width if fixed
	int fixed_w;
};
#endif
