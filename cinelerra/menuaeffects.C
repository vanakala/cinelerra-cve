// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "edl.h"
#include "menuaeffects.h"
#include "mwindow.h"
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
	mwindow_global->add_aeffect_menu(title);
}

MenuAEffectItem::MenuAEffectItem(MenuAEffects *menueffect, const char *string)
 : MenuEffectItem(menueffect, string)
{
}
