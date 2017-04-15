
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "clip.h"
#include "loadmode.h"
#include "mwindow.h"
#include "selection.h"
#include "theme.h"
#include "language.h"

const struct selection_int InsertionModeSelection::insertion_modes[] =
{
	{ N_("Insert nothing"), LOADMODE_NOTHING },
	{ N_("Replace current project"), LOADMODE_REPLACE },
	{ N_("Replace current project and concatenate tracks"), LOADMODE_REPLACE_CONCATENATE },
	{ N_("Append in new tracks"), LOADMODE_NEW_TRACKS },
	{ N_("Concatenate to existing tracks"), LOADMODE_CONCATENATE },
	{ N_("Paste at insertion point"), LOADMODE_CONCATENATE },
	{ N_("Create new resources only"), LOADMODE_RESOURCESONLY },
	{ 0, 0 }
};

#define NUM_INSMODES (sizeof(InsertionModeSelection::insertion_modes) / sizeof(struct selection_int) - 1)


LoadMode::LoadMode(BC_WindowBase *window,
	int x, 
	int y, 
	int *output, 
	int use_nothing)
{
	window->add_subwindow(title = new BC_Title(x, y, _("Insertion strategy:")));
	y += 20;
	window->add_subwindow(modeselection = new InsertionModeSelection(x, y,
		window, output,
		use_nothing ? LOADMODE_MASK_ALL : LOADMODE_MASK_NONO));
	modeselection->update(*output);
}

void LoadMode::reposition_window(int x, int y)
{
	title->reposition_window(x, y);
	y += 20;
	modeselection->reposition_window(x, y);
}

InsertionModeSelection::InsertionModeSelection(int x, int y,
	BC_WindowBase *base, int *value, int optmask)
 : Selection(x, y , base, insertion_modes, value,
	SELECTION_VARWIDTH | SELECTION_VARNUMITEMS | optmask)
{
	disable(1);
}

void InsertionModeSelection::update(int value)
{
	BC_TextBox::update(mode_to_text(value));
}

const char* InsertionModeSelection::mode_to_text(int mode)
{
	for(int i = 0; i < NUM_INSMODES; i++)
	{
		if(insertion_modes[i].value == mode)
			return _(insertion_modes[i].text);
	}
	return _("Unknown");
}
