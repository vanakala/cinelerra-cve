
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
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "menuveffects.h"
#include "patchbay.h"
#include "quicktime.h"
#include "tracks.h"
#include "units.h"
#include "vpluginarray.h"


MenuVEffects::MenuVEffects(MWindow *mwindow)
 : MenuEffects(mwindow)
{
	thread = new MenuVEffectThread(mwindow);
}

MenuVEffects::~MenuVEffects()
{
	delete thread;
}

MenuVEffectThread::MenuVEffectThread(MWindow *mwindow)
 : MenuEffectThread(mwindow)
{
}

MenuVEffectThread::~MenuVEffectThread()
{
}

int MenuVEffectThread::get_recordable_tracks(Asset *asset)
{
	asset->layers = mwindow->edl->tracks->recordable_video_tracks();
	return asset->layers;
}

int MenuVEffectThread::get_derived_attributes(Asset *asset, BC_Hash *defaults)
{

	asset->load_defaults(defaults, 
		"VEFFECT_",
		1, 
		1,
		1,
		0,
		0);




// Fix asset for video only
	if(!File::supports_video(asset->format)) asset->format = FILE_MOV;
	asset->audio_data = 0;
	asset->video_data = 1;



	return 0;
}

int MenuVEffectThread::save_derived_attributes(Asset *asset, BC_Hash *defaults)
{
	asset->save_defaults(defaults,
		"VEFFECT_",
		1,
		1,
		1,
		0,
		0);


	return 0;
}

PluginArray* MenuVEffectThread::create_plugin_array()
{
	return new VPluginArray();
}

int64_t MenuVEffectThread::to_units(double position, int round)
{
	if(round)
		return Units::round(position * mwindow->edl->session->frame_rate);
	else
		return (int64_t)(position * mwindow->edl->session->frame_rate);
		
	return 0;
}

int MenuVEffectThread::fix_menu(char *title)
{
	mwindow->gui->mainmenu->add_veffect(title); 
}

MenuVEffectItem::MenuVEffectItem(MenuVEffects *menueffect, char *string)
 : MenuEffectItem(menueffect, string)
{
}
