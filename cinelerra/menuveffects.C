
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
#include "menuveffects.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "renderprofiles.inc"


MenuVEffects::MenuVEffects()
 : MenuEffects()
{
	thread = new MenuVEffectThread();
}

MenuVEffects::~MenuVEffects()
{
	delete thread;
}

MenuVEffectThread::MenuVEffectThread()
 : MenuEffectThread()
{
	effect_type = SUPPORTS_VIDEO;
	track_type = TRACK_VIDEO;
	def_prefix = "VEFFECT_";
	profile_name = RENDERCONFIG_VEFFECT;
}

int MenuVEffectThread::get_recordable_tracks(Asset *asset)
{
	asset->layers = master_edl->recordable_tracks_of(TRACK_VIDEO);
	return asset->layers;
}

void MenuVEffectThread::fix_menu(const char *title)
{
	mwindow_global->gui->mainmenu->add_veffect(title);
}

MenuVEffectItem::MenuVEffectItem(MenuVEffects *menueffect, const char *string)
 : MenuEffectItem(menueffect, string)
{
}
