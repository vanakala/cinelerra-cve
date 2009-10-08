
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

#ifndef MENUAEFFECTS_H
#define MENUAEFFECTS_H

#include "asset.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "menueffects.h"
#include "pluginserver.inc"

class MenuAEffects : public MenuEffects
{
public:
	MenuAEffects(MWindow *mwindow);
	~MenuAEffects();
};

class MenuAEffectThread : public MenuEffectThread
{
public:
	MenuAEffectThread(MWindow *mwindow);
	~MenuAEffectThread();

	int get_recordable_tracks(Asset *asset);
	int get_derived_attributes(Asset *asset, BC_Hash *defaults);
	int save_derived_attributes(Asset *asset, BC_Hash *defaults);
	PluginArray* create_plugin_array();
	int64_t to_units(double position, int round);
	int fix_menu(char *title);
};


class MenuAEffectItem : public MenuEffectItem
{
public:
	MenuAEffectItem(MenuAEffects *menueffect, char *string);
};




#endif
