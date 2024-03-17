// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2015 Einar Rünkaru <einarrunkaru at gmail dot com>

#include "bctitle.h"
#include "bcsignals.h"
#include "clip.h"
#include "loadmode.h"
#include "mwindow.h"
#include "selection.h"
#include "theme.h"
#include "language.h"

const struct selection_int LoadMode::insertion_modes[] =
{
	{ N_("Insert nothing"), LOADMODE_NOTHING },
	{ N_("Replace current project"), LOADMODE_REPLACE },
	{ N_("Replace current project and concatenate tracks"), LOADMODE_REPLACE_CONCATENATE },
	{ N_("Append in new tracks"), LOADMODE_NEW_TRACKS },
	{ N_("Concatenate to existing tracks"), LOADMODE_CONCATENATE },
	{ N_("Paste at insertion point"), LOADMODE_PASTE },
	{ N_("Create new resources only"), LOADMODE_RESOURCESONLY },
	{ 0, 0 }
};

#define NUM_INSMODES (sizeof(LoadMode::insertion_modes) / sizeof(struct selection_int) - 1)


LoadMode::LoadMode(BC_WindowBase *window, int x, int y, int *output, int use_nothing)
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

const char* LoadMode::name(int mode)
{
	for(int i = 0; i < NUM_INSMODES; i++)
	{
		if(insertion_modes[i].value == mode)
			return _(insertion_modes[i].text);
	}
	return _("Unknown");
}


InsertionModeSelection::InsertionModeSelection(int x, int y,
	BC_WindowBase *base, int *value, int optmask)
 : Selection(x, y , base, LoadMode::insertion_modes, value,
	SELECTION_VARWIDTH | SELECTION_VARNUMITEMS | optmask)
{
	disable(1);
}

void InsertionModeSelection::update(int value)
{
	BC_TextBox::update(LoadMode::name(value));
}
