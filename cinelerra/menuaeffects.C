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
