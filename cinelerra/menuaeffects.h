// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MENUAEFFECTS_H
#define MENUAEFFECTS_H

#include "menuaeffects.inc"
#include "menueffects.h"

class MenuAEffects : public MenuEffects
{
public:
	MenuAEffects();
	~MenuAEffects();
};


class MenuAEffectThread : public MenuEffectThread
{
public:
	MenuAEffectThread();

	int get_recordable_tracks(Asset *asset);
	void fix_menu(const char *title);
};


class MenuAEffectItem : public MenuEffectItem
{
public:
	MenuAEffectItem(MenuAEffects *menueffect, const char *string);
};

#endif
