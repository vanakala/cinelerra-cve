#include "apluginarray.h"
#include "assets.h"
#include "defaults.h"
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


int MenuAEffectThread::get_derived_attributes(Asset *asset, Defaults *defaults)
{
	char string[1024];
	File file;
	defaults->get("AEFFECTPATH", asset->path);
	sprintf(string, "WAV");
	defaults->get("AEFFECTFORMAT", string);
	if(!file.supports_audio(mwindow->plugindb, string)) sprintf(string, WAV_NAME);
	asset->format = file.strtoformat(mwindow->plugindb, string);
//printf("MenuAEffectThread::get_derived_attributes %s\n", string);
	asset->sample_rate = mwindow->edl->session->sample_rate;
	asset->bits = defaults->get("AEFFECTBITS", 16);
	dither = defaults->get("AEFFECTDITHER", 0);
	asset->signed_ = defaults->get("AEFFECTSIGNED", 1);
	asset->byte_order = defaults->get("AEFFECTBYTEORDER", 1);
	asset->audio_data = 1;





	asset->load_defaults(defaults);
	defaults->get("EFFECT_AUDIO_CODEC", asset->acodec);
	return 0;
}

int MenuAEffectThread::save_derived_attributes(Asset *asset, Defaults *defaults)
{
	File file;
	defaults->update("AEFFECTPATH", asset->path);
	defaults->update("AEFFECTFORMAT", file.formattostr(mwindow->plugindb, asset->format));
//printf("MenuAEffectThread::save_derived_attributes %s\n", file.formattostr(mwindow->plugindb, asset->format));
	defaults->update("AEFFECTBITS", asset->bits);
	defaults->update("AEFFECTDITHER", dither);
	defaults->update("AEFFECTSIGNED", asset->signed_);
	defaults->update("AEFFECTBYTEORDER", asset->byte_order);

	asset->save_defaults(defaults);
	defaults->update("EFFECT_AUDIO_CODEC", asset->acodec);
	return 0;
}


PluginArray* MenuAEffectThread::create_plugin_array()
{
	return new APluginArray();
}

long MenuAEffectThread::to_units(double position, int round)
{
	if(round)
		return Units::round(position * mwindow->edl->session->sample_rate);
	else
		return (long)(position * mwindow->edl->session->sample_rate);
		
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
