#include "audioconfig.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "new.h"
#include "preferences.h"
#include "recordconfig.h"
#include "vdeviceprefs.h"
#include "videoinprefs.h"

VideoInPrefs::VideoInPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	this->mwindow = mwindow;
}

VideoInPrefs::~VideoInPrefs()
{
}

int VideoInPrefs::create_objects()
{
	int x = 5, y = 5;
	char string[1024];
	BC_TextBox *textbox;


	add_subwindow(new BC_Title(x, y, "Video In", LARGEFONT, BLACK));
	y += 25;

	add_subwindow(new BC_Title(x, y, "Record Driver:", MEDIUMFONT, BLACK));
	video_in_device = new VDevicePrefs(x + 110, 
		y, 
		pwindow, 
		this, 
		0, 
		pwindow->thread->edl->session->vconfig_in, 
		MODERECORD);
	video_in_device->initialize();

	y += 55;
	sprintf(string, "%d", pwindow->thread->edl->session->video_write_length);
	add_subwindow(textbox = new VideoWriteLength(pwindow, string, y));
	add_subwindow(new CaptureLengthTumbler(pwindow, textbox, textbox->get_x() + textbox->get_w(), y));
	add_subwindow(new BC_Title(x, y, "Frames to record to disk at a time:"));
	y += 27;
	sprintf(string, "%d", pwindow->thread->edl->session->vconfig_in->capture_length);
	add_subwindow(textbox = new VideoCaptureLength(pwindow, string, y));
	add_subwindow(new CaptureLengthTumbler(pwindow, textbox, textbox->get_x() + textbox->get_w(), y));
	add_subwindow(new BC_Title(x, y, "Frames to buffer in device:"));
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
	add_subwindow(new BC_Title(x, y, "Size of captured frame:"));
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

	y += 27;
	x = 5;
	add_subwindow(new BC_Title(x, y, "Frame rate is always the default frame rate of the project."));
	return 0;
}



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
 : BC_CheckBox(x, y, value, "Use software for positioning information")
{
	this->pwindow = pwindow; 
}

int RecordSoftwareTimer::handle_event() 
{
	pwindow->thread->edl->session->record_software_position = get_value(); 
	return 1;
}



RecordSyncDrives::RecordSyncDrives(PreferencesWindow *pwindow, int value, int x, int y)
 : BC_CheckBox(x, y, value, "Sync drives automatically")
{
	this->pwindow = pwindow; 
}

int RecordSyncDrives::handle_event() 
{
	pwindow->thread->edl->session->record_sync_drives = get_value(); 
	return 1;
}



