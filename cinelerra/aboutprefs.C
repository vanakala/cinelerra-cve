#include "aboutprefs.h"
#include "builddate.h"
#include "language.h"
#include "libmpeg3.h"
#include "mwindow.h"
#include "quicktime.h"
#include "theme.h"
#include "vframe.h"



AboutPrefs::AboutPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
}

AboutPrefs::~AboutPrefs()
{
}

int AboutPrefs::create_objects()
{
	int x = 5, y = 20;







	char license1[BCTEXTLEN];
	sprintf(license1, "%s %s", _("Cinelerra "), CINELERRA_VERSION);

	set_font(LARGEFONT);
	set_color(BLACK);
	draw_text(x, y, license1);
	y += get_text_height(LARGEFONT);

	char license2[BCTEXTLEN];
	sprintf(license2, "%s%s%s", 
		_("(c) 2003 Heroine Virtual Ltd.\n\n"),
		_("Build date: "), 
		BUILDDATE);
	set_font(MEDIUMFONT);
	draw_text(x, y, license2);



	y += get_text_height(MEDIUMFONT) * 3;

	char versions[BCTEXTLEN];
	sprintf(versions, 
_("Quicktime version %d.%d.%d\n"
"Libmpeg3 version %d.%d.%d\n"),
quicktime_major(),
quicktime_minor(),
quicktime_release(),
mpeg3_major(),
mpeg3_minor(),
mpeg3_release());
	draw_text(x, y, versions);



	y += get_text_height(MEDIUMFONT) * 3;
	set_font(LARGEFONT);
	draw_text(x, y, "Credits:");
	y += get_text_height(LARGEFONT);
	set_font(MEDIUMFONT);

	char credits[BCTEXTLEN];
	sprintf(credits,
"Richard Baverstock\n"
"Alex Ferrer\n"
"Eric Seigne\n"
"Andraz Tori\n"
"\"ga\"\n"
);
	draw_text(x, y, credits);

	y = 450;

	set_font(LARGEFONT);
	draw_text(x, y, "License:");
	y += get_text_height(LARGEFONT);

	set_font(MEDIUMFONT);

	char license3[BCTEXTLEN];
	sprintf(license3, _(
"This program is free software; you can redistribute it and/or modify it under the terms\n"
"of the GNU General Public License as published by the Free Software Foundation; either version\n"
"2 of the License, or (at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\n"
"without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR\n"
"PURPOSE.  See the GNU General Public License for more details.\n"
"\n"));
	draw_text(x, y, license3);

	x = get_w() - mwindow->theme->about_bg->get_w() - 10;
	y = 5;
	BC_Pixmap *temp_pixmap = new BC_Pixmap(this, 
		mwindow->theme->about_bg,
		PIXMAP_ALPHA);
	draw_pixmap(temp_pixmap, 
		x, 
		y);

	delete temp_pixmap;


	x += mwindow->theme->about_bg->get_w() + 10;
	y += get_text_height(LARGEFONT) * 2;


	flash();
	flush();
	return 0;
}


