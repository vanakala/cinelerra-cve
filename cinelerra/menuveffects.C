// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "edl.h"
#include "menuveffects.h"
#include "mwindow.h"
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
	mwindow_global->add_veffect_menu(title);
}

MenuVEffectItem::MenuVEffectItem(MenuVEffects *menueffect, const char *string)
 : MenuEffectItem(menueffect, string)
{
}
