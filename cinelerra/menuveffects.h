// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MENUVEFFECTS_H
#define MENUVEFFECTS_H

#include "menueffects.h"

class MenuVEffects : public MenuEffects
{
public:
	MenuVEffects();
	~MenuVEffects();
};


class MenuVEffectThread : public MenuEffectThread
{
public:
	MenuVEffectThread();

	int get_recordable_tracks(Asset *asset);
	void fix_menu(const char *title);
};


class MenuVEffectItem : public MenuEffectItem
{
public:
	MenuVEffectItem(MenuVEffects *menueffect, const char *string);
};

#endif
