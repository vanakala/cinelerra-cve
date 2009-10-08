
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "adeviceprefs.h"
#include "audioconfig.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "formattools.h"
#include "new.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "recordconfig.h"
#include "recordprefs.h"
#include "theme.h"
#include "vdeviceprefs.h"




RecordPrefs::RecordPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	this->mwindow = mwindow;
}

RecordPrefs::~RecordPrefs()
{
	delete audio_in_device;
	delete recording_format;
//	delete duplex_device;
}

int RecordPrefs::create_objects()
{
	int x, y, x2;
	char string[BCTEXTLEN];
	BC_Resources *resources = BC_WindowBase::get_resources();
	BC_Title *title;

	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;

	add_subwindow(title = new BC_Title(x, 
		y, 
		_("File Format:"), 
		LARGEFONT, 
		resources->text_default));
	y += title->get_h() + 5;

	recording_format = 
		new FormatTools(mwindow,
			this, 
			pwindow->thread->edl->session->recording_format);
	recording_format->create_objects(x, 
		y, 
		1,  // Include tools for audio
		1,  // Include tools for video
		1,  // Include checkbox for audio
		1,  // Include checkbox for video
		0,
		1,
		0,  // Select compressors to be offered
		1,  // Prompt for recording options
		0,  // If nonzero, prompt for insertion strategy
		0); // Supply file formats for background rendering





// Audio hardware
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;


	add_subwindow(title = new BC_Title(x, 
		y, 
		_("Audio In"), 
		LARGEFONT, 
		resources->text_default));

	y += title->get_h() + 5;

	add_subwindow(new BC_Title(x, y, _("Record Driver:"), MEDIUMFONT, resources->text_default));
	audio_in_device = new ADevicePrefs(x + 110, 
		y, 
		pwindow, 
		this, 
		0,
		pwindow->thread->edl->session->aconfig_in, 
		MODERECORD);
	audio_in_device->initialize(1);
	y += audio_in_device->get_h(1);


	BC_TextBox *textbox;
	BC_Title *title1, *title2, *title3;
	add_subwindow(title1 = new BC_Title(x, y, _("Samples to write to disk at a time:")));
	add_subwindow(title2 = new BC_Title(x, y + 30, _("Sample rate for recording:")));
	add_subwindow(title3 = new BC_Title(x, y + 60, _("Channels to record:")));
	x2 = MAX(title1->get_w(), title2->get_w()) + 10;
	x2 = MAX(x2, title3->get_w() + 10);

	sprintf(string, "%ld", pwindow->thread->edl->session->record_write_length);
	add_subwindow(textbox = new RecordWriteLength(mwindow, 
		pwindow, 
		x2, 
		y, 
		string));
	add_subwindow(textbox = new RecordSampleRate(pwindow, x2, y + 30));
	add_subwindow(new SampleRatePulldown(mwindow, textbox, x2 + textbox->get_w(), y + 30));

	RecordChannels *channels = new RecordChannels(pwindow, this, x2, y + 60);
	channels->create_objects();

	y += 90;


	add_subwindow(new RecordRealTime(mwindow, 
		pwindow, 
		x, 
		y, 
		pwindow->thread->edl->session->real_time_record));
	y += 45;
	x = 5;




// Video hardware
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;


	add_subwindow(new BC_Title(x, y, _("Video In"), LARGEFONT, resources->text_default));
	y += 25;

	add_subwindow(new BC_Title(x, 
		y, 
		_("Record Driver:"), 
		MEDIUMFONT, 
		resources->text_default));
	video_in_device = new VDevicePrefs(x + 110, 
		y, 
		pwindow, 
		this, 
		0, 
		pwindow->thread->edl->session->vconfig_in, 
		MODERECORD);
	video_in_device->initialize(1);

	y += 55;
	sprintf(string, "%d", pwindow->thread->edl->session->video_write_length);
	add_subwindow(textbox = new VideoWriteLength(pwindow, string, y));
	add_subwindow(new CaptureLengthTumbler(pwindow, textbox, textbox->get_x() + textbox->get_w(), y));
	add_subwindow(new BC_Title(x, y, _("Frames to record to disk at a time:")));
	y += 27;
	sprintf(string, "%d", pwindow->thread->edl->session->vconfig_in->capture_length);
	add_subwindow(textbox = new VideoCaptureLength(pwindow, string, y));
	add_subwindow(new CaptureLengthTumbler(pwindow, textbox, textbox->get_x() + textbox->get_w(), y));
	add_subwindow(new BC_Title(x, y, _("Frames to buffer in device:")));
	y += 27;

	add_subwindow(new RecordSoftwareTimer(pwindow, 
		pwindow->thread->edl->session->record_software_position, 
		x, 
		y));
	y += 27;
	add_subwindow(new RecordSyncDrives(pwindow, 
		pwindow->thread->edl->session->record_sync_drives, 
		x, 
		y));
	y += 35;

	BC_TextBox *w_text, *h_text;
	add_subwindow(new BC_Title(x, y, _("Size of captured frame:")));
	x += 170;
	add_subwindow(w_text = new RecordW(pwindow, x, y));
	x += w_text->get_w() + 2;
	add_subwindow(new BC_Title(x, y, "x"));
	x += 10;
	add_subwindow(h_text = new RecordH(pwindow, x, y));
	x += h_text->get_w();
	add_subwindow(new FrameSizePulldown(mwindow, 
		w_text, 
		h_text, 
		x, 
		y));

	y += 30;
	x = 5;
	add_subwindow(new BC_Title(x, y, _("Frame rate for recording:")));
	x += 180;
	add_subwindow(textbox = new RecordFrameRate(pwindow, x, y));
	x += 75;
	add_subwindow(new FrameRatePulldown(mwindow, textbox, x, y));
	y += 45;
	x = 5;
	
	add_subwindow(new BC_Title(x, y, _("Images"), LARGEFONT, BLACK));
	y += 25;
	add_subwindow(new StillImageUseDuration(pwindow, 
		pwindow->thread->edl->session->si_useduration, 
		x, 
		y));
	x += 260;
	y += 5;
	add_subwindow(new StillImageDuration(pwindow, x, y));
	x += 80;
	y += 5;
	add_subwindow(new BC_Title(x, y, _("Seconds")));
	y += 30;

	return 0;
}


RecordWriteLength::RecordWriteLength(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
}

int RecordWriteLength::handle_event()
{ 
	pwindow->thread->edl->session->record_write_length = atol(get_text());
	return 1; 
}



RecordRealTime::RecordRealTime(MWindow *mwindow, 
	PreferencesWindow *pwindow, 
	int x, 
	int y, 
	int value)
 : BC_CheckBox(x, 
 	y, 
	value, 
	_("Record in realtime priority (root only)"))
{ 
	this->pwindow = pwindow; 
}

int RecordRealTime::handle_event()
{
	pwindow->thread->edl->session->real_time_record = get_value();
}


RecordSampleRate::RecordSampleRate(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->aconfig_in->in_samplerate)
{
	this->pwindow = pwindow;
}
int RecordSampleRate::handle_event()
{
	pwindow->thread->edl->session->aconfig_in->in_samplerate = atol(get_text());
	return 1;
}


// DuplexEnable::DuplexEnable(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value)
//  : BC_CheckBox(x, y, value, _("Enable full duplex"))
// { this->pwindow = pwindow; }
// 
// int DuplexEnable::handle_event()
// {
// 	pwindow->thread->edl->session->enable_duplex = get_value();
// }
// 


RecordW::RecordW(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->vconfig_in->w)
{
	this->pwindow = pwindow;
}
int RecordW::handle_event()
{
	pwindow->thread->edl->session->vconfig_in->w = atol(get_text());
	return 1;
}

RecordH::RecordH(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->vconfig_in->h)
{
	this->pwindow = pwindow;
}
int RecordH::handle_event()
{
	pwindow->thread->edl->session->vconfig_in->h = atol(get_text());
	return 1;
}

RecordFrameRate::RecordFrameRate(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->vconfig_in->in_framerate)
{
	this->pwindow = pwindow;
}
int RecordFrameRate::handle_event()
{
	pwindow->thread->edl->session->vconfig_in->in_framerate = atof(get_text());
	return 1;
}



RecordChannels::RecordChannels(PreferencesWindow *pwindow, BC_SubWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, 
		pwindow->thread->edl->session->aconfig_in->channels,
		1,
		MAX_CHANNELS,
		x, 
		y, 
		100)
{
	this->pwindow = pwindow; 
}

int RecordChannels::handle_event()
{
	pwindow->thread->edl->session->aconfig_in->channels = atoi(get_text());
	return 1;
}



VideoWriteLength::VideoWriteLength(PreferencesWindow *pwindow, char *text, int y)
 : BC_TextBox(260, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
}

int VideoWriteLength::handle_event()
{ 
	pwindow->thread->edl->session->video_write_length = atol(get_text()); 
	return 1;
}


VideoCaptureLength::VideoCaptureLength(PreferencesWindow *pwindow, char *text, int y)
 : BC_TextBox(260, y, 100, 1, text)
{ 
	this->pwindow = pwindow;
}

int VideoCaptureLength::handle_event()
{ 
	pwindow->thread->edl->session->vconfig_in->capture_length = atol(get_text()); 
	return 1; 
}

CaptureLengthTumbler::CaptureLengthTumbler(PreferencesWindow *pwindow, BC_TextBox *text, int x, int y)
 : BC_Tumbler(x, y)
{
	this->pwindow;
	this->text = text;
}

int CaptureLengthTumbler::handle_up_event()
{
	int value = atol(text->get_text());
	value++;
	char string[BCTEXTLEN];
	sprintf(string, "%d", value);
	text->update(string);
	text->handle_event();
	return 1;
}

int CaptureLengthTumbler::handle_down_event()
{
	int value = atol(text->get_text());
	value--;
	value = MAX(1, value);
	char string[BCTEXTLEN];
	sprintf(string, "%d", value);
	text->update(string);
	text->handle_event();
	return 1;
}



RecordSoftwareTimer::RecordSoftwareTimer(PreferencesWindow *pwindow, int value, int x, int y)
 : BC_CheckBox(x, y, value, _("Use software for positioning information"))
{
	this->pwindow = pwindow; 
}

int RecordSoftwareTimer::handle_event() 
{
	pwindow->thread->edl->session->record_software_position = get_value(); 
	return 1;
}



RecordSyncDrives::RecordSyncDrives(PreferencesWindow *pwindow, int value, int x, int y)
 : BC_CheckBox(x, y, value, _("Sync drives automatically"))
{
	this->pwindow = pwindow; 
}

int RecordSyncDrives::handle_event() 
{
	pwindow->thread->edl->session->record_sync_drives = get_value(); 
	return 1;
}

StillImageUseDuration::StillImageUseDuration(PreferencesWindow *pwindow, int value, int x, int y)
 : BC_CheckBox(x, y, value, _("Import images with a duration of"))
{ 
	this->pwindow = pwindow; 
}

int StillImageUseDuration::handle_event()
{
	pwindow->thread->edl->session->si_useduration = get_value();
}

StillImageDuration::StillImageDuration(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->si_duration)
{
	this->pwindow = pwindow;
}
int StillImageDuration::handle_event()
{
	pwindow->thread->edl->session->si_duration = atof(get_text());
	return 1;
}


