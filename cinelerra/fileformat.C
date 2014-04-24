
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

#include "asset.h"
#include "assets.h"
#include "fileformat.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "selection.h"
#include "theme.h"
#include "new.h"


FileFormat::FileFormat(MWindow *mwindow, Asset *asset, const char *string2)
 : BC_Window("File Format - " PROGRAM_NAME,
		mwindow->gui->get_abs_cursor_x(1),
		mwindow->gui->get_abs_cursor_y(1),
		375, 
		300, 
		375, 
		300)
{
	char string[1024];
	int x1 = 10, x2 = 180;
	int x = x1, y = 10;
	SampleBitsSelection *bitspopup;

	this->mwindow = mwindow;
	this->asset = asset;

	add_subwindow(new BC_Title(x, y, string2));
	y += 20;
	add_subwindow(new BC_Title(x, y, _("Assuming raw PCM:")));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Channels:")));
	sprintf(string, "%d", asset->channels);
	channels_button = new FileFormatChannels(x2, y, this, string);

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Sample rate:")));
	add_subwindow(rate_button = new SampleRateSelection(x2, y, this,
		&asset->sample_rate));
	rate_button->update(asset->sample_rate);

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Bits:")));
	add_subwindow(bitspopup = new SampleBitsSelection(x2, y, this, &asset->bits,
		SBITS_LINEAR| SBITS_ULAW | SBITS_ADPCM));
	bitspopup->update_size(asset->bits);

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Header length:")));
	sprintf(string, "%d", asset->header);
	add_subwindow(header_button = new FileFormatHeader(x2, y, this, string));

	y += 30;

	add_subwindow(new BC_Title(x, y, _("Byte order:")));
	add_subwindow(lohi = new FileFormatByteOrderLOHI(x2, y, this, asset->byte_order));
	add_subwindow(hilo = new FileFormatByteOrderHILO(x2 + 70, y, this, !asset->byte_order));

	y += 30;
	add_subwindow(signed_button = new FileFormatSigned(x, y, this, asset->signed_));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
}

FileFormat::~FileFormat()
{
	delete lohi;
	delete hilo;
	delete signed_button;
	delete header_button;
	delete rate_button;
	delete channels_button;
}


FileFormatChannels::FileFormatChannels(int x, int y, FileFormat *fwindow, char *text)
 : BC_TumbleTextBox(fwindow, 
	(int)atol(text), 
	(int)1, 
	(int)MAXCHANNELS, 
	x, 
	y, 
	50)
{
	this->fwindow = fwindow;
}

int FileFormatChannels::handle_event()
{
	fwindow->asset->channels = atol(get_text());
	return 1;
}


FileFormatHeader::FileFormatHeader(int x, int y, FileFormat *fwindow, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->fwindow = fwindow;
}

int FileFormatHeader::handle_event()
{
	fwindow->asset->header = atol(get_text());
	return 1;
}


FileFormatByteOrderLOHI::FileFormatByteOrderLOHI(int x, int y, FileFormat *fwindow, int value)
 : BC_Radial(x, y, value, _("Lo Hi"))
{
	this->fwindow = fwindow;
}

int FileFormatByteOrderLOHI::handle_event()
{
	update(1);
	fwindow->asset->byte_order = 1;
	fwindow->hilo->update(0);
	return 1;
}


FileFormatByteOrderHILO::FileFormatByteOrderHILO(int x, int y, FileFormat *fwindow, int value)
 : BC_Radial(x, y, value, _("Hi Lo"))
{
	this->fwindow = fwindow;
}

int FileFormatByteOrderHILO::handle_event()
{
	update(1);
	fwindow->asset->byte_order = 0;
	fwindow->lohi->update(0);
	return 1;
}


FileFormatSigned::FileFormatSigned(int x, int y, FileFormat *fwindow, int value)
 : BC_CheckBox(x, y, value, _("Values are signed"))
{
	this->fwindow = fwindow;
}

int FileFormatSigned::handle_event()
{
	fwindow->asset->signed_ = get_value();
	return 1;
}
