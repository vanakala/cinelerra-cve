
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

#include "asset.h"
#include "bcsignals.h"
#include "edl.h"
#include "mainmenu.h"
#include "menuaeffects.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "renderprofiles.inc"

// ============================================= audio effects

MenuAEffects::MenuAEffects()
 : MenuEffects()
{
	thread = new MenuAEffectThread();
}

MenuAEffects::~MenuAEffects()
{
	delete thread;
}


MenuAEffectThread::MenuAEffectThread()
 : MenuEffectThread()
{
	effect_type = SUPPORTS_AUDIO;
	track_type = TRACK_AUDIO;
	def_prefix = "AEFFECT_";
	profile_name = RENDERCONFIG_AEFFECT;
}

int MenuAEffectThread::get_recordable_tracks(Asset *asset)
{
	asset->channels = master_edl->recordable_tracks_of(TRACK_AUDIO);
	return asset->channels;
}

void MenuAEffectThread::fix_menu(const char *title)
{
	mwindow_global->gui->mainmenu->add_aeffect(title);
}

MenuAEffectItem::MenuAEffectItem(MenuAEffects *menueffect, const char *string)
 : MenuEffectItem(menueffect, string)
{
}
