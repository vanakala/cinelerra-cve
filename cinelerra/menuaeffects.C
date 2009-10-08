
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
}

MenuAEffectThread::~MenuAEffectThread()
{
}

int MenuAEffectThread::get_recordable_tracks(Asset *asset)
{
	asset->channels = mwindow->edl->tracks->recordable_audio_tracks();
	return asset->channels;
}


int MenuAEffectThread::get_derived_attributes(Asset *asset, BC_Hash *defaults)
{
	asset->load_defaults(defaults, 
		"AEFFECT_",
		1, 
		1,
		1,
		0,
		1);


// Fix asset for audio only
	if(!File::supports_audio(asset->format)) asset->format = FILE_WAV;
	asset->audio_data = 1;
	asset->video_data = 0;

	return 0;
}

int MenuAEffectThread::save_derived_attributes(Asset *asset, BC_Hash *defaults)
{
	asset->save_defaults(defaults, 
		"AEFFECT_",
		1, 
		1,
		1,
		0,
		1);

	return 0;
}


PluginArray* MenuAEffectThread::create_plugin_array()
{
	return new APluginArray();
}

int64_t MenuAEffectThread::to_units(double position, int round)
{
	if(round)
		return Units::round(position * mwindow->edl->session->sample_rate);
	else
		return (int64_t)(position * mwindow->edl->session->sample_rate);
		
	return 0;
}

int MenuAEffectThread::fix_menu(char *title)
{
	mwindow->gui->mainmenu->add_aeffect(title); 
}



MenuAEffectItem::MenuAEffectItem(MenuAEffects *menueffect, char *string)
 : MenuEffectItem(menueffect, string)
{
}
