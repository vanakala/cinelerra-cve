#include "adeviceprefs.h"
#include "audioconfig.h"
#include "audioinprefs.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "new.h"
#include "preferences.h"
#include "recordconfig.h"
#include "vdeviceprefs.h"

AudioInPrefs::AudioInPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	this->mwindow = mwindow;
}

AudioInPrefs::~AudioInPrefs()
{
	delete in_device;
	delete duplex_device;
}

int AudioInPrefs::create_objects()
{
	int x = 5, y = 5;
	char string[1024];

	add_subwindow(new BC_Title(x, y, "Audio In", LARGEFONT, BLACK));
	y += 25;


	add_subwindow(new BC_Title(x, y, "Record Driver:", MEDIUMFONT, BLACK));
	in_device = new ADevicePrefs(x + 110, 
		y, 
		pwindow, 
		this, 
		0,
		pwindow->thread->edl->session->aconfig_in, 
		MODERECORD);
	in_device->initialize();

	y += ADevicePrefs::get_h();

	add_subwindow(new BC_Title(x, y, "Duplex Driver:", MEDIUMFONT, BLACK));
	duplex_device = new ADevicePrefs(x + 110, 
		y, 
		pwindow, 
		this, 
		pwindow->thread->edl->session->aconfig_duplex, 
		0,
		MODEDUPLEX);
	duplex_device->initialize();

	y += ADevicePrefs::get_h();
	BC_TextBox *textbox;
	add_subwindow(new BC_Title(x, y, "Samples to write to disk at a time:"));
	sprintf(string, "%ld", pwindow->thread->edl->session->record_write_length);
	add_subwindow(textbox = new RecordWriteLength(mwindow, pwindow, x + 240, y, string));

	y += 30;
	add_subwindow(new BC_Title(x, y, "Be sure to allocate enough samples to allow video compression."));
	y += 30;
	add_subwindow(new DuplexEnable(mwindow, pwindow, x, y, pwindow->thread->edl->session->enable_duplex));
	y += 30;
	add_subwindow(new RecordRealTime(mwindow, pwindow, x, y, pwindow->thread->edl->session->real_time_record));
	y += 45;


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



RecordRealTime::RecordRealTime(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value)
 : BC_CheckBox(x, y, value, "Record in realtime priority (root only)")
{ 
	this->pwindow = pwindow; 
}

int RecordRealTime::handle_event()
{
	pwindow->thread->edl->session->real_time_record = get_value();
}

DuplexEnable::DuplexEnable(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value)
 : BC_CheckBox(x, y, value, "Enable full duplex")
{ this->pwindow = pwindow; }

int DuplexEnable::handle_event()
{
	pwindow->thread->edl->session->enable_duplex = get_value();
}
