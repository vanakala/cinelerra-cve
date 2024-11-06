// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2015 Einar Rünkaru <einarrunkaru at gmail dot com>

#include "bcbar.h"
#include "bctitle.h"
#include "bcresources.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "miscprefs.h"
#include "preferences.h"
#include "selection.h"
#include "theme.h"

#include <string.h>

MiscPrefs::MiscPrefs(PreferencesWindow *pwindow)
 : PreferencesDialog(pwindow)
{
}

void MiscPrefs::show()
{
	BC_WindowBase *win;
	int x0, y0, boxh;
	int x = theme_global->preferencesoptions_x;
	int y = theme_global->preferencesoptions_y;

	add_subwindow(new BC_Title(x, y, _("Images"), LARGEFONT, get_resources()->text_default));
	y += 30;
	win = add_subwindow(new CheckBox(x, y, _("Import images with a duration of"),
		&pwindow->thread->this_edlsession->si_useduration));
	x += win->get_w() + 10;
	win = add_subwindow(new DblValueTextBox(x, y,
		&pwindow->thread->this_edlsession->si_duration));
	x += win->get_w() + 10;
	boxh = win->get_h() + 5;
	y += 3;
	add_subwindow(new BC_Title(x, y, _("seconds")));
	x = theme_global->preferencesoptions_x;
	y += win->get_h() + 5;
	add_subwindow(new BC_Title(x, y, _("AVlibs"), LARGEFONT, get_resources()->text_default));
	y += 30;
	win = add_subwindow(new CheckBox(x, y, _("Show messages of avlibs in terminal"),
		&pwindow->thread->this_edlsession->show_avlibsmsgs));
	y += win->get_h() + 5;
	win = add_subwindow(new CheckBox(x, y, _("Allow using experimental codecs"),
		&pwindow->thread->this_edlsession->experimental_codecs));
	y += win->get_h() + 5;
	win = add_subwindow(new CheckBox(x, y, _("Show another menu of encoders"),
		&pwindow->thread->this_edlsession->encoders_menu));
	y0 = y += win->get_h() + 5;
	win = add_subwindow(new CheckBox(x, y, _("Calculate framerate"),
		&pwindow->thread->this_edlsession->calculate_framerate));
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
	x = x0 + theme_global->preferencesoptions_x;
	y = y0;
	win = add_subwindow(new TextBox(x, y, pwindow->thread->this_edlsession->metadata_author));
	y += boxh;
	win = add_subwindow(new TextBox(x, y, pwindow->thread->this_edlsession->metadata_title));
	y += boxh;
	win = add_subwindow(new TextBox(x, y, pwindow->thread->this_edlsession->metadata_copyright));

	y += 35;
	add_subwindow(new BC_Bar(5, y,  get_w() - 10));
	y += 25;
	x = theme_global->preferencesoptions_x;
	win = add_subwindow(new CheckBox(x, y, _("Automatic backups with interval"),
		&pwindow->thread->this_edlsession->automatic_backups));
	x += win->get_w() + 10;
	win = add_subwindow(new ValueTextBox(x, y,
		&pwindow->thread->this_edlsession->backup_interval));
	x += win->get_w() + 10;
	y += 3;
	add_subwindow(new BC_Title(x, y, _("seconds")));

	y += 35;
	x = theme_global->preferencesoptions_x;
	add_subwindow(new CheckBox(x, y, _("Shrink tracks of invisible plugins"),
		&pwindow->thread->this_edlsession->shrink_plugin_tracks));

	y += 35;
	win = add_subwindow(new BC_Title(x, y, _("Output color depth:")));
	x += win->get_w() + 10;
	OutputDepthSelection *depth_selection;
	add_subwindow(depth_selection = new OutputDepthSelection(x, y, this,
		&pwindow->thread->this_edlsession->output_color_depth));
	depth_selection->update(pwindow->thread->this_edlsession->output_color_depth);
}
