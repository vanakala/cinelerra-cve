#ifndef MENUAEFFECTS_H
#define MENUAEFFECTS_H

#include "assets.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "menueffects.h"
#include "pluginserver.inc"

class MenuAEffects : public MenuEffects
{
public:
	MenuAEffects(MWindow *mwindow);
	~MenuAEffects();
};

class MenuAEffectThread : public MenuEffectThread
{
public:
	MenuAEffectThread(MWindow *mwindow);
	~MenuAEffectThread();

	int get_recordable_tracks(Asset *asset);
	int get_derived_attributes(Asset *asset, Defaults *defaults);
	int save_derived_attributes(Asset *asset, Defaults *defaults);
	PluginArray* create_plugin_array();
	int64_t to_units(double position, int round);
	int fix_menu(char *title);
};


class MenuAEffectItem : public MenuEffectItem
{
public:
	MenuAEffectItem(MenuAEffects *menueffect, char *string);
};




#endif
