#include "assets.h"
#include "defaults.h"
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

int MenuVEffectThread::get_derived_attributes(Asset *asset, Defaults *defaults)
{
	char string[1024];
	File file;
	defaults->get("VEFFECTPATH", asset->path);
	sprintf(string, MOV_NAME);
	defaults->get("VEFFECTFORMAT", string);
	if(!file.supports_video(mwindow->plugindb, string)) sprintf(string, MOV_NAME);
	asset->format = file.strtoformat(mwindow->plugindb, string);
	sprintf(asset->vcodec, QUICKTIME_YUV2);
	asset->video_data = 1;



	asset->load_defaults(defaults);
	defaults->get("EFFECT_VIDEO_CODEC", asset->vcodec);
	return 0;
}

int MenuVEffectThread::save_derived_attributes(Asset *asset, Defaults *defaults)
{
	File file;
	defaults->update("VEFFECTPATH", asset->path);
	defaults->update("VEFFECTFORMAT", file.formattostr(mwindow->plugindb, asset->format));
	defaults->update("VEFFECTCOMPRESSION", asset->vcodec);


	asset->save_defaults(defaults);
	defaults->update("EFFECT_VIDEO_CODEC", asset->vcodec);
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
