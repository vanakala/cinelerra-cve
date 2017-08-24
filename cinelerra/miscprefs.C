/*
 * CINELERRA
 * Copyright (C) 2015 Einar Rünkaru <einarrunkaru at gmail dot com>
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

#include "bctitle.h"
#include "bcresources.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "miscprefs.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"

#include <string.h>

MiscPrefs::MiscPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
}

void MiscPrefs::show()
{
	BC_WindowBase *win;
	int x0, y0, boxh;
	int x = mwindow->theme->preferencesoptions_x;
	int y = mwindow->theme->preferencesoptions_y;

	add_subwindow(new BC_Title(x, y, _("Images"), LARGEFONT, get_resources()->text_default));
	y += 30;
	win = add_subwindow(new StillImageUseDuration(pwindow,
		pwindow->thread->edl->session->si_useduration,
		x,
		y));
	x += win->get_w() + 10;
	win = add_subwindow(new StillImageDuration(pwindow, x, y));
	x += win->get_w() + 10;
	boxh = win->get_h() + 5;
	y += 3;
	add_subwindow(new BC_Title(x, y, _("seconds")));
	x = mwindow->theme->preferencesoptions_x;
	y += win->get_h() + 5;
	add_subwindow(new BC_Title(x, y, _("AVlibs"), LARGEFONT, get_resources()->text_default));
	y += 30;
	win = add_subwindow(new ToggleButton(x, y, _("Show messages of avlibs in terminal"),
		&pwindow->thread->edl->session->show_avlibsmsgs));
	y += win->get_h() + 5;
	win = add_subwindow(new ToggleButton(x, y, _("Allow using experimental codecs"),
		&pwindow->thread->edl->session->experimental_codecs));
	y0 = y += win->get_h() + 5;
	win = add_subwindow(new BC_Title(x, y, _("Author:")));
	x0 = win->get_w() + 10;
	y += boxh;
	win = add_subwindow(new BC_Title(x, y, _("Title:")));
	if(x0 < win->get_w() + 10)
		x0 = win->get_w() + 10;
	y += boxh;
	win = add_subwindow(new BC_Title(x, y, _("Copyright:")));
	if(x0 < win->get_w() + 10)
		x0 = win->get_w() + 10;
	x = x0 + mwindow->theme->preferencesoptions_x;
	y = y0;
	win = add_subwindow(new MiscText(x, y, pwindow->thread->edl->session->metadata_author));
	y += boxh;
	win = add_subwindow(new MiscText(x, y, pwindow->thread->edl->session->metadata_title));
	y += boxh;
	win = add_subwindow(new MiscText(x, y, pwindow->thread->edl->session->metadata_copyright));
}

StillImageUseDuration::StillImageUseDuration(PreferencesWindow *pwindow, int value, int x, int y)
 : BC_CheckBox(x, y, value, _("Import images with a duration of"))
{
	this->pwindow = pwindow; 
}

int StillImageUseDuration::handle_event()
{
	pwindow->thread->edl->session->si_useduration = get_value();
}


StillImageDuration::StillImageDuration(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->si_duration)
{
	this->pwindow = pwindow;
}

int StillImageDuration::handle_event()
{
	pwindow->thread->edl->session->si_duration = atof(get_text());
	return 1;
}

ToggleButton::ToggleButton(int x, int y, const char *text, int *value)
 : BC_CheckBox(x, y, *value, text)
{
	valueptr = value;
}

int ToggleButton::handle_event()
{
	*valueptr = get_value();
}

MiscText::MiscText(int x, int y, char *boxtext)
 : BC_TextBox(x, y, 400, 1, boxtext)
{
	this->str = boxtext;
}

int MiscText::handle_event()
{
	strcpy(str, get_text());
}
