
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

#ifndef AWINDOW_H
#define AWINDOW_H

#include "assetedit.inc"
#include "assetremove.inc"
#include "awindowgui.inc"
#include "bcwindowbase.inc"
#include "clipedit.inc"
#include "labeledit.inc"
#include "mwindow.inc"
#include "thread.h"

class AWindow : public Thread
{
public:
	AWindow(MWindow *mwindow);
	~AWindow();

	void run();
	int create_objects();

	char current_folder[BCTEXTLEN];

	AWindowGUI *gui;
	MWindow *mwindow;
	AssetEdit *asset_edit;
	AssetRemoveThread *asset_remove;
	ClipEdit *clip_edit;
	LabelEdit *label_edit;
};

#endif
