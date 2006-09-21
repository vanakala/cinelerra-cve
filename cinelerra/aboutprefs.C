#include "aboutprefs.h"
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
	int x, y;

	BC_Resources *resources = BC_WindowBase::get_resources();

// 	add_subwindow(new BC_Title(mwindow->theme->preferencestitle_x, 
// 		mwindow->theme->preferencestitle_y, 
// 		_("About"), 
// 		LARGEFONT, 
// 		resources->text_default));
	
	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y +
		get_text_height(LARGEFONT);

	char license1[BCTEXTLEN];
	sprintf(license1, "%s %s", _("Cinelerra "), CINELERRA_VERSION);

	set_font(LARGEFONT);
	set_color(resources->text_default);
	draw_text(x, y, license1);

	y += get_text_height(LARGEFONT);
	char license2[BCTEXTLEN];
	sprintf(license2, "%s%s%s%s%s", 
		_("(C) 2006 Heroine Virtual Ltd.\n\n"),
		_("SVN Version: "),
		SVNVERSION,
		_("\nBuild date: "), 
		BUILDDATE);
	set_font(MEDIUMFONT);
	draw_text(x, y, license2);



	y += get_text_height(MEDIUMFONT) * 4;

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

"Jack Crossfire\n"
"Richard Baverstock\n"
"Karl Bielefeldt\n"
"Kevin Brosius\n"
"Jean-Luc Coulon\n"
"Jean-Michel POURE\n"
"Jerome Cornet\n"
"Pierre Marc Dumuid\n"
"Alex Ferrer\n"
"Jan Gerber\n"
"Koen Muylkens\n"
"Stefan de Konink\n"
"Nathan Kurz\n"
"Greg Mekkes\n"
"Eric Seigne\n"
"Joe Stewart\n"
"Dan Streetman\n"
"Gustavo Iñiguez\n"
"Johannes Sixt\n"
"Mark Taraba\n"
"Andraz Tori\n"
"Jonas Wulff\n"

);
	draw_text(x, y, credits);

	y = get_h() - 135;

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
	y = mwindow->theme->preferencesoptions_y;
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


