
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
#include "apluginarray.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "menuaeffects.h"
#include "patchbay.h"
#include "renderprofiles.inc"
#include "tracks.h"

// ============================================= audio effects

MenuAEffects::MenuAEffects(MWindow *mwindow)
 : MenuEffects(mwindow)
{
	thread = new MenuAEffectThread(mwindow);
}

MenuAEffects::~MenuAEffects()
{
	delete thread;
}


MenuAEffectThread::MenuAEffectThread(MWindow *mwindow)
 : MenuEffectThread(mwindow)
{
	effect_type = SUPPORTS_AUDIO;
	def_prefix = "AEFFECT_";
	profile_name = RENDERCONFIG_AEFFECT;
}

int MenuAEffectThread::get_recordable_tracks(Asset *asset)
{
	asset->channels = master_edl->tracks->recordable_audio_tracks();
	return asset->channels;
}

PluginArray* MenuAEffectThread::create_plugin_array()
{
	return new APluginArray();
}

ptstime MenuAEffectThread::one_unit()
{
	return (ptstime)1 / edlsession->sample_rate;
}

posnum MenuAEffectThread::to_units(ptstime position, int round)
{
	if(round)
		return Units::round(position * edlsession->sample_rate);
	else
		return (posnum)(position * edlsession->sample_rate);
}

void MenuAEffectThread::fix_menu(const char *title)
{
	mwindow->gui->mainmenu->add_aeffect(title); 
}


MenuAEffectItem::MenuAEffectItem(MenuAEffects *menueffect, const char *string)
 : MenuEffectItem(menueffect, string)
{
}
