#include "adeviceprefs.h"
#include "audioconfig.h"
#include "audiodevice.inc"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mwindow.h"
#include "overlayframe.inc"
#include "playbackprefs.h"
#include "preferences.h"
#include "theme.h"
#include "vdeviceprefs.h"
#include "videodevice.inc"



PlaybackPrefs::PlaybackPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	video_device = 0;
}

PlaybackPrefs::~PlaybackPrefs()
{
	delete audio_device;
	delete video_device;
}

int PlaybackPrefs::create_objects()
{
	int x, y, x2;
	char string[BCTEXTLEN];
	BC_PopupTextBox *popup;
	BC_Resources *resources = BC_WindowBase::get_resources();

	playback_config = pwindow->thread->edl->session->playback_config;

// Audio
	add_subwindow(new BC_Title(mwindow->theme->preferencestitle_x, 
		mwindow->theme->preferencestitle_y, 
		_("Audio Out"), 
		LARGEFONT, 
		resources->text_default));
	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;



	BC_Title *title1, *title2;
	add_subwindow(title2 = new BC_Title(x, y, _("Samples to send to console at a time:"), MEDIUMFONT, resources->text_default));
	x2 = MAX(title2->get_w(), title2->get_w()) + 10;

SET_TRACE
	sprintf(string, "%d", playback_config->aconfig->fragment_size);
	PlaybackModuleFragment *menu;
	add_subwindow(menu = new PlaybackModuleFragment(x2, 
		y, 
		pwindow, 
		this, 
		string));
	menu->add_item(new BC_MenuItem("2048"));
	menu->add_item(new BC_MenuItem("4096"));
	menu->add_item(new BC_MenuItem("8192"));
	menu->add_item(new BC_MenuItem("16384"));
	menu->add_item(new BC_MenuItem("32768"));
	menu->add_item(new BC_MenuItem("65536"));
	menu->add_item(new BC_MenuItem("131072"));
	menu->add_item(new BC_MenuItem("262144"));

SET_TRACE
	y += menu->get_h() + 5;
	x2 = x;
	add_subwindow(title1 = new BC_Title(x2, y, _("Audio offset (sec):")));
	x2 += title1->get_w() + 5;
	PlaybackAudioOffset *audio_offset = new PlaybackAudioOffset(pwindow,
		this,
		x2,
		y);
	audio_offset->create_objects();
	y += audio_offset->get_h() + 5;

SET_TRACE
	add_subwindow(new PlaybackViewFollows(pwindow, pwindow->thread->edl->session->view_follows_playback, y));
	y += 30;
	add_subwindow(new PlaybackSoftwareTimer(pwindow, pwindow->thread->edl->session->playback_software_position, y));
	y += 30;
	add_subwindow(new PlaybackRealTime(pwindow, pwindow->thread->edl->session->real_time_playback, y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Audio Driver:")));
	audio_device = new ADevicePrefs(x + 100, 
		y, 
		pwindow, 
		this, 
		playback_config->aconfig, 
		0,
		MODEPLAY);
	audio_device->initialize();

SET_TRACE



// Video
 	y += audio_device->get_h();

SET_TRACE
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;

SET_TRACE
	add_subwindow(new BC_Title(x, y, _("Video Out"), LARGEFONT, resources->text_default));
	y += 30;

SET_TRACE
	add_subwindow(new VideoEveryFrame(pwindow, x, y));

	add_subwindow(new BC_Title(x + 200, y + 10, _("Framerate achieved:")));
	add_subwindow(framerate_title = new BC_Title(x + 350, y + 10, _("--"), MEDIUMFONT, RED));
	draw_framerate();

	y += 35;

SET_TRACE
 	add_subwindow(new BC_Title(x, y, _("Scaling equation:")));
	y += 20;
	add_subwindow(nearest_neighbor = new PlaybackNearest(pwindow, 
		this, 
		pwindow->thread->edl->session->interpolation_type == NEAREST_NEIGHBOR, 
		10, 
		y));
	y += 20;
	add_subwindow(cubic_linear = new PlaybackBicubicBilinear(pwindow, 
		this, 
		pwindow->thread->edl->session->interpolation_type == CUBIC_LINEAR, 
		10, 
		y));
	y += 20;
	add_subwindow(linear_linear = new PlaybackBilinearBilinear(pwindow, 
		this, 
		pwindow->thread->edl->session->interpolation_type == LINEAR_LINEAR, 
		10, 
		y));

SET_TRACE
	y += 35;
	add_subwindow(new BC_Title(x, y, _("Preload buffer for Quicktime:"), MEDIUMFONT, resources->text_default));
	sprintf(string, "%d", pwindow->thread->edl->session->playback_preload);
	add_subwindow(new PlaybackPreload(x + 210, y, pwindow, this, string));

SET_TRACE
//	y += 30;
//	add_subwindow(new PlaybackDeblock(pwindow, 10, y));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Timecode offset:"), MEDIUMFONT, BLACK));
	sprintf(string, "%d", pwindow->thread->edl->session->timecode_offset[3]);
	add_subwindow(new TimecodeOffset(x + 120, y, pwindow, this, string, 3));
	add_subwindow(new BC_Title(x + 152, y, _(":"), MEDIUMFONT, BLACK));
	sprintf(string, "%d", pwindow->thread->edl->session->timecode_offset[2]);
	add_subwindow(new TimecodeOffset(x + 160, y, pwindow, this, string, 2));
	add_subwindow(new BC_Title(x + 192, y, _(":"), MEDIUMFONT, BLACK));
	sprintf(string, "%d", pwindow->thread->edl->session->timecode_offset[1]);
	add_subwindow(new TimecodeOffset(x + 200, y, pwindow, this, string, 1));
	add_subwindow(new BC_Title(x + 232, y, _(":"), MEDIUMFONT, BLACK));
	sprintf(string, "%d", pwindow->thread->edl->session->timecode_offset[0]);
	add_subwindow(new TimecodeOffset(x + 240, y, pwindow, this, string, 0));

	y += 35;
	add_subwindow(vdevice_title = new BC_Title(x, y, _("Video Driver:")));
	video_device = new VDevicePrefs(x + 100, 
		y, 
		pwindow, 
		this, 
		playback_config->vconfig, 
		0,
		MODEPLAY);
	video_device->initialize();

SET_TRACE	

	return 0;
}


void PlaybackPrefs::update(int interpolation)
{
	pwindow->thread->edl->session->interpolation_type = interpolation;
	nearest_neighbor->update(interpolation == NEAREST_NEIGHBOR);
//	cubic_cubic->update(interpolation == CUBIC_CUBIC);
	cubic_linear->update(interpolation == CUBIC_LINEAR);
	linear_linear->update(interpolation == LINEAR_LINEAR);
}


int PlaybackPrefs::get_buffer_bytes()
{
//	return pwindow->thread->edl->aconfig->oss_out_bits / 8 * pwindow->thread->preferences->aconfig->oss_out_channels * pwindow->thread->preferences->playback_buffer;
}

int PlaybackPrefs::draw_framerate()
{
//printf("PlaybackPrefs::draw_framerate 1 %f\n", pwindow->thread->edl->session->actual_frame_rate);
	char string[BCTEXTLEN];
	sprintf(string, "%.4f", pwindow->thread->edl->session->actual_frame_rate);
	framerate_title->update(string);
	return 0;
}



PlaybackAudioOffset::PlaybackAudioOffset(PreferencesWindow *pwindow, 
	PlaybackPrefs *playback, 
	int x, 
	int y)
 : BC_TumbleTextBox(playback,
 	playback->playback_config->aconfig->audio_offset,
	-10.0,
	10.0,
	x,
	y,
	100)
{
	this->pwindow = pwindow;
	this->playback = playback;
	set_precision(2);
	set_increment(0.1);
}

int PlaybackAudioOffset::handle_event()
{
	playback->playback_config->aconfig->audio_offset = atof(get_text());
	return 1;
}




PlaybackModuleFragment::PlaybackModuleFragment(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback, 
	char *text)
 : BC_PopupMenu(x, 
 	y, 
	100, 
	text,
	1)
{ 
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackModuleFragment::handle_event() 
{
	playback->playback_config->aconfig->fragment_size = atol(get_text()); 
	return 1;
}




PlaybackViewFollows::PlaybackViewFollows(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("View follows playback"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackViewFollows::handle_event() 
{ 
	pwindow->thread->edl->session->view_follows_playback = get_value(); 
	return 1;
}




PlaybackSoftwareTimer::PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Use software for positioning information"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackSoftwareTimer::handle_event() 
{ 
	pwindow->thread->edl->session->playback_software_position = get_value(); 
	return 1;
}




PlaybackRealTime::PlaybackRealTime(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Audio playback in real time priority (root only)"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackRealTime::handle_event() 
{ 
	pwindow->thread->edl->session->real_time_playback = get_value(); 
	return 1;
}







PlaybackNearest::PlaybackNearest(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Nearest neighbor enlarge and reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackNearest::handle_event()
{
	prefs->update(NEAREST_NEIGHBOR);
	return 1;
}





PlaybackBicubicBicubic::PlaybackBicubicBicubic(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Bicubic enlarge and reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackBicubicBicubic::handle_event()
{
	prefs->update(CUBIC_CUBIC);
	return 1;
}




PlaybackBicubicBilinear::PlaybackBicubicBilinear(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Bicubic enlarge and bilinear reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackBicubicBilinear::handle_event()
{
	prefs->update(CUBIC_LINEAR);
	return 1;
}


PlaybackBilinearBilinear::PlaybackBilinearBilinear(PreferencesWindow *pwindow, 
	PlaybackPrefs *prefs, 
	int value, 
	int x, 
	int y)
 : BC_Radial(x, y, value, _("Bilinear enlarge and bilinear reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackBilinearBilinear::handle_event()
{
	prefs->update(LINEAR_LINEAR);
	return 1;
}


PlaybackPreload::PlaybackPreload(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback, 
	char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
	this->playback = playback; 
}

int PlaybackPreload::handle_event() 
{ 
	pwindow->thread->edl->session->playback_preload = atol(get_text()); 
	return 1;
}



VideoEveryFrame::VideoEveryFrame(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->edl->session->video_every_frame, _("Play every frame"))
{
	this->pwindow = pwindow;
}

int VideoEveryFrame::handle_event()
{
	pwindow->thread->edl->session->video_every_frame = get_value();
	return 1;
}














TimecodeOffset::TimecodeOffset(int x, int y, PreferencesWindow *pwindow, 
      PlaybackPrefs *playback, char *text, int unit)
 : BC_TextBox(x, y, 30, 1, text)
{
   this->pwindow = pwindow;
   this->playback = playback;
	this->unit = unit;
}

int TimecodeOffset::handle_event()
{
	pwindow->thread->edl->session->timecode_offset[unit] = atol(get_text());
	return 1;
}




