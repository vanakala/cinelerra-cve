#include "assets.h"
#include "bitspopup.h"
#include "file.h"
#include "filesystem.h"
#include "fileformat.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"

FileFormat::FileFormat(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": File Format", 
		mwindow->gui->get_abs_cursor_x(),
		mwindow->gui->get_abs_cursor_y(),
 		375, 
		300, 
		375, 
		300)
{
	this->mwindow = mwindow;
}

FileFormat::~FileFormat()
{
	delete lohi;
	delete hilo;
	delete signed_button;
	delete header_button;
	delete rate_button;
	delete channels_button;
	delete bitspopup;
}

int FileFormat::create_objects(Asset *asset, char *string2)
{
// ================================= copy values
	this->asset = asset;
	create_objects_(string2);
}

int FileFormat::create_objects_(char *string2)
{
	FileSystem dir;
	File file;
	char string[1024];
	int x1 = 10, x2 = 180;
	int x = x1, y = 10;
	add_subwindow(new BC_Title(x, y, string2));
	y += 20;
	add_subwindow(new BC_Title(x, y, "Assuming raw PCM:"));

	y += 30;
	add_subwindow(new BC_Title(x, y, "Channels:"));
	sprintf(string, "%d", asset->channels);
	channels_button = new FileFormatChannels(x2, y, this, string);
	channels_button->create_objects();

	y += 30;
	add_subwindow(new BC_Title(x, y, "Sample rate:"));
	sprintf(string, "%d", asset->sample_rate);
	add_subwindow(rate_button = new FileFormatRate(x2, y, this, string));
	add_subwindow(new SampleRatePulldown(mwindow, rate_button, x2 + 100, y));
	
	y += 30;
	add_subwindow(new BC_Title(x, y, "Bits:"));
	bitspopup = new BitsPopup(this, 
		x2, 
		y, 
		&asset->bits, 
		0, 
		1, 
		1, 
		0, 
		1);
	bitspopup->create_objects();
	
	y += 30;
	add_subwindow(new BC_Title(x, y, "Header length:"));
	sprintf(string, "%d", asset->header);
	add_subwindow(header_button = new FileFormatHeader(x2, y, this, string));
	
	y += 30;
	add_subwindow(new BC_Title(x, y, "Byte order:"));
	add_subwindow(lohi = new FileFormatByteOrderLOHI(x2, y, this, asset->byte_order));
	add_subwindow(hilo = new FileFormatByteOrderHILO(x2 + 70, y, this, asset->byte_order ^ 1));
	
	y += 30;
	add_subwindow(signed_button = new FileFormatSigned(x, y, this, asset->signed_));
	
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	return 0;
}

FileFormatChannels::FileFormatChannels(int x, int y, FileFormat *fwindow, char *text)
 : BC_TumbleTextBox(fwindow, 
 	atol(text), 
	1, 
	MAXCHANNELS, 
	x, 
	y, 
	50)
{
	this->fwindow = fwindow;
}

int FileFormatChannels::handle_event()
{
	fwindow->asset->channels = atol(get_text());
	return 0;
}

FileFormatRate::FileFormatRate(int x, int y, FileFormat *fwindow, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->fwindow = fwindow;
}

int FileFormatRate::handle_event()
{
	fwindow->asset->sample_rate = atol(get_text());
	return 0;
}

FileFormatHeader::FileFormatHeader(int x, int y, FileFormat *fwindow, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->fwindow = fwindow;
}

int FileFormatHeader::handle_event()
{
	fwindow->asset->header = atol(get_text());
	return 0;
}

FileFormatByteOrderLOHI::FileFormatByteOrderLOHI(int x, int y, FileFormat *fwindow, int value)
 : BC_Radial(x, y, value, "Lo Hi")
{
	this->fwindow = fwindow;
}

int FileFormatByteOrderLOHI::handle_event()
{
	fwindow->asset->byte_order = get_value();
	fwindow->hilo->update(get_value() ^ 1);
}

FileFormatByteOrderHILO::FileFormatByteOrderHILO(int x, int y, FileFormat *fwindow, int value)
 : BC_Radial(x, y, value, "Hi Lo")
{
	this->fwindow = fwindow;
}

int FileFormatByteOrderHILO::handle_event()
{
	fwindow->asset->byte_order = get_value() ^ 1;
	fwindow->lohi->update(get_value() ^ 1);
}

FileFormatSigned::FileFormatSigned(int x, int y, FileFormat *fwindow, int value)
 : BC_CheckBox(x, y, value, "Values are signed")
{
	this->fwindow = fwindow;
}

int FileFormatSigned::handle_event()
{
	fwindow->asset->signed_ = get_value();
}
