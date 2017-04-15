
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "aboutprefs.h"
#include "bcwindowbase.h"
#include "bcresources.h"
#include "bcbitmap.h"
#include "bcpixmap.h"
#include "language.h"
#include "libmpeg3.h"
#include "mwindow.h"
#include "theme.h"
#include "vframe.h"
#include "versioninfo.h"


AboutPrefs::AboutPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
}

void AboutPrefs::show()
{
	int x, y;
        char strbuf[BCTEXTLEN];

	BC_Resources *resources = BC_WindowBase::get_resources();

	x = get_w() - mwindow->theme->about_bg->get_w() - 10;
	y = mwindow->theme->preferencesoptions_y;

	BC_Pixmap *temp_pixmap = new BC_Pixmap(this, 
		mwindow->theme->about_bg,
		PIXMAP_ALPHA);
	draw_pixmap(temp_pixmap, x, y);

	delete temp_pixmap;

	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y +
		get_text_height(LARGEFONT);

	set_font(LARGEFONT);
	set_color(resources->text_default);
	draw_text(x, y, PROGRAM_NAME " " CINELERRA_VERSION);

	y += get_text_height(LARGEFONT);
	set_font(MEDIUMFONT);

	draw_text(x, y, COPYRIGHTTEXT1
#if defined(COPYRIGHTTEXT2)
	"\n" COPYRIGHTTEXT2
#endif
#if defined(REPOMAINTXT)
	"\n" REPOMAINTXT
#endif
	);

	y += get_text_height(MEDIUMFONT) * 3;

	sprintf(strbuf, _("%s\nLibmpeg3 version %d.%d.%d\n"),
		FFMPEG_EXTERNALTEXT,
		mpeg3_major(),
		mpeg3_minor(),
		mpeg3_release());
	draw_text(x, y, strbuf);

	y += get_text_height(MEDIUMFONT) * 4;
	set_font(LARGEFONT);
	draw_text(x, y, "Credits:");
	y += get_text_height(LARGEFONT);
	set_font(MEDIUMFONT);

	draw_utf8_text(x, y,
		"Jack Crossfire\n"
		"Richard Baverstock\n"
		"Karl Bielefeldt\n"
		"Kevin Brosius\n"
		"Jean-Luc Coulon\n"
		"Jean-Michel Poure\n"
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
		"Dan Streetman\n");

	draw_utf8_text(x + 180, y,
		"Gustavo Iñiguez\n"
		"Johannes Sixt\n"
		"Mark Taraba\n"
		"Andraz Tori\n"
		"Jonas Wulff\n"
		"David Arendt\n"
		"Einar Rünkaru\n"
		"Monty Montgomery\n");

	y = get_h() - 135;

	set_font(LARGEFONT);
	draw_text(x, y, "License:");
	y += get_text_height(LARGEFONT);

	set_font(MEDIUMFONT);

	draw_text(x, y, _(
		"This program is free software; you can redistribute it and/or modify it under the terms\n"
		"of the GNU General Public License as published by the Free Software Foundation; either version\n"
		"2 of the License, or (at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\n"
		"without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR\n"
		"PURPOSE.  See the GNU General Public License for more details.\n"
		"\n"));

	x += mwindow->theme->about_bg->get_w() + 10;
	y += get_text_height(LARGEFONT) * 2;

	flash();
	flush();
}
