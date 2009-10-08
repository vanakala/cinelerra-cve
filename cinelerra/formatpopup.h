
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

#ifndef FORMATPOPUP_H
#define FORMATPOPUP_H



#include "guicast.h"
#include "formatpopup.inc"
#include "pluginserver.inc"

class FormatPopup : public BC_ListBox
{
public:
	FormatPopup(ArrayList<PluginServer*> *plugindb, 
		int x, 
		int y,
		int use_brender = 0); // Show formats useful in background rendering
	~FormatPopup();

	int create_objects();
	virtual int handle_event();  // user copies text to value here
	ArrayList<PluginServer*> *plugindb;
	ArrayList<BC_ListBoxItem*> format_items;
	int use_brender;
};






#endif
