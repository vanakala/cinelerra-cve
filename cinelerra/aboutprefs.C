#include "aboutprefs.h"
#include "builddate.h"
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
	int x = 5, y = 5;






	BC_Pixmap *temp_pixmap = new BC_Pixmap(this, 
		mwindow->theme->about_bg,
		PIXMAP_ALPHA);
	draw_pixmap(temp_pixmap, 
		x, 
		y);
// 	draw_vframe(mwindow->theme->about_bg,
// 		x, 
// 		y);


// 	BC_Pixmap *temp_pixmap2 = new BC_Pixmap(this, 
// 		mwindow->theme->about_microsoft,
// 		PIXMAP_ALPHA);
// 	draw_pixmap(temp_pixmap2, 
// 		x, 
// 		y + mwindow->theme->about_bg->get_h());

	delete temp_pixmap;
//	delete temp_pixmap2;


	x += mwindow->theme->about_bg->get_w() + 10;
	y += get_text_height(LARGEFONT) * 2;

	char *license1 = 
"Cinelerra " VERSION;

	set_font(LARGEFONT);
	set_color(BLACK);
	draw_text(x, y, license1);


	char *license2 = 
"(c) 2002 Heroine Virtual Ltd.\n\n"
"Build date: " BUILDDATE;
	y += get_text_height(LARGEFONT) * 2;
	set_font(MEDIUMFONT);
	draw_text(x, y, license2);




	char *license3 = 
"This program is free software; you can\n"
"redistribute it and/or modify it under the terms\n"
"of the GNU General Public License as published\n"
"by  the Free Software Foundation; either version\n"
"2 of the License, or (at your option) any later\n"
"version.\n"
"\n"
"This program is distributed in the hope that it\n"
"will be useful, but WITHOUT ANY WARRANTY;\n"
"without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR\n"
"PURPOSE.  See the GNU General Public License for\n"
"more details.\n"
"\n";
	y += 100;
	draw_text(x, y, license3);


	char versions[BCTEXTLEN];
	sprintf(versions, 
"Quicktime version %d.%d.%d\n"
"Libmpeg3 version %d.%d.%d\n",
quicktime_major(),
quicktime_minor(),
quicktime_release(),
mpeg3_major(),
mpeg3_minor(),
mpeg3_release());
	y += 250;
	draw_text(10, y, versions);



	flash();
	flush();
	return 0;
}


