#ifndef MENUVEFFECTS_H
#define MENUVEFFECTS_H

#include "asset.inc"
#include "edl.inc"
#include "mwindow.inc"
#include "menueffects.h"
#include "pluginserver.inc"

class MenuVEffects : public MenuEffects
{
public:
	MenuVEffects(MWindow *mwindow);
	~MenuVEffects();
};

class MenuVEffectThread : public MenuEffectThread
{
public:
	MenuVEffectThread(MWindow *mwindow);
	~MenuVEffectThread();

	int get_recordable_tracks(Asset *asset);
	int get_derived_attributes(Asset *asset, Defaults *defaults);
	int save_derived_attributes(Asset *asset, Defaults *defaults);
	PluginArray* create_plugin_array();
	int fix_menu(char *title);

	int64_t to_units(double position, int round);
};

class MenuVEffectItem : public MenuEffectItem
{
public:
	MenuVEffectItem(MenuVEffects *menueffect, char *string);
};

#endif
