#include "autoconf.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "viewmenu.h"
#include "trackcanvas.h"


ShowEdits::ShowEdits(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Show edits"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
	set_checked(0); 
}
int ShowEdits::handle_event()
{
//	mwindow->tracks->toggle_handles();
	set_checked(get_checked() ^ 1);
	return 1;
}

ShowTitles::ShowTitles(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Show titles"), hotkey, hotkey[0])
{
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->show_titles); 
}
int ShowTitles::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->show_titles = get_checked();
	mwindow->gui->update(1,
		1,
		0,
		0,
		1, 
		0,
		0);
	return 1;
}


ShowKeyframes::ShowKeyframes(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Plugin keyframes"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
//	set_checked(mwindow->edl->session->auto_conf->keyframes); 
}
int ShowKeyframes::handle_event()
{
	return 1;
}


ShowTransitions::ShowTransitions(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Show transitions"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->auto_conf->transitions); 
}
int ShowTransitions::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->transitions = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}


FadeAutomation::FadeAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Fade keyframes"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->auto_conf->fade); 
} 
int FadeAutomation::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->fade = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}

#if 0
PlayAutomation::PlayAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Play keyframes"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->auto_conf->play); 
} 
int PlayAutomation::handle_event()
{ 
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->play = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}
#endif


CameraAutomation::CameraAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Camera keyframes"), hotkey, hotkey[0]) 
{ 
	this->mwindow = mwindow; 
	set_checked(0); 
} 
int CameraAutomation::handle_event()
{ 
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->camera = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}


ProjectAutomation::ProjectAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Projector keyframes"), hotkey, hotkey[0]) 
{ 
	this->mwindow = mwindow; 
	set_checked(0); 
}
int ProjectAutomation::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->projector = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}


ShowRenderedOutput::ShowRenderedOutput(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Draw Output"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
	set_checked(0); 
}
int ShowRenderedOutput::handle_event()
{
//	mwindow->tracks->set_draw_output();
//	if(mwindow->tracks->show_output) set_text(_("Draw Tracks")); else set_text(_("Draw Output"));
	return 1;
}


MuteAutomation::MuteAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Mute keyframes"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->auto_conf->mute); 
}
int MuteAutomation::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->mute = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}


PanAutomation::PanAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Pan keyframes"), hotkey, hotkey[0]) 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->auto_conf->pan); 
}
int PanAutomation::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->pan = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}

PluginAutomation::PluginAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Plugin keyframes"), hotkey, hotkey[0]) 
{ 
	this->mwindow = mwindow; 
}

int PluginAutomation::handle_event()
{
	set_checked(!get_checked());
	mwindow->edl->session->auto_conf->plugins = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}

ModeAutomation::ModeAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Mode keyframes"), hotkey, hotkey[0]) 
{ 
	this->mwindow = mwindow;
}

int ModeAutomation::handle_event()
{
	set_checked(!get_checked());
	mwindow->edl->session->auto_conf->mode = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}

MaskAutomation::MaskAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Mask keyframes"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow;
}

int MaskAutomation::handle_event()
{
	set_checked(!get_checked());
	mwindow->edl->session->auto_conf->mask = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}

CZoomAutomation::CZoomAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Camera Zoom"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow;
}

int CZoomAutomation::handle_event()
{
	set_checked(!get_checked());
	mwindow->edl->session->auto_conf->czoom = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}

PZoomAutomation::PZoomAutomation(MWindow *mwindow, char *hotkey)
 : BC_MenuItem(_("Projector Zoom"), hotkey, hotkey[0])
{ 
	set_shift(1); 
	this->mwindow = mwindow;
}

int PZoomAutomation::handle_event()
{
	set_checked(!get_checked());
	mwindow->edl->session->auto_conf->pzoom = get_checked();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->gui->mainmenu->draw_items();
	return 1;
}
