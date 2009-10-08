
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

#include "compresspopup.h"
#include "file.h"
#include "language.h"
#include "quicktime.h"
#include <string.h>



CompressPopup::CompressPopup(int x, int y, int use_dv, char *text)
 : BC_PopupMenu(x, y, 80, File::compressiontostr(text))
{
	strcpy(format, text);
	this->use_dv = use_dv;
}

int CompressPopup::add_items()
{
	if(!use_dv) add_item(format_items[0] = new CompressPopupItem(_("DV")));
	add_item(format_items[1] = new CompressPopupItem(_("JPEG")));
	add_item(format_items[2] = new CompressPopupItem(_("MJPA")));
	add_item(format_items[3] = new CompressPopupItem(_("PNG")));
	add_item(format_items[4] = new CompressPopupItem(_("PNG-Alpha")));
	add_item(format_items[5] = new CompressPopupItem(_("RGB")));
	add_item(format_items[6] = new CompressPopupItem(_("RGB-Alpha")));
	add_item(format_items[7] = new CompressPopupItem(_("YUV420")));
	add_item(format_items[8] = new CompressPopupItem(_("YUV422")));
	return 0;
}

CompressPopup::~CompressPopup()
{
	for(int i = 0; i < COMPRESS_ITEMS; i++) delete format_items[i];
}

char* CompressPopup::get_compression()
{
	File test_file;
	return test_file.strtocompression(get_text());
}

char* CompressPopup::compression_to_text(char *compression)
{
	File test_file;
	return test_file.compressiontostr(compression);
}

CompressPopupItem::CompressPopupItem(char *text)
 : BC_MenuItem(text)
{
}

CompressPopupItem::~CompressPopupItem()
{
}
	
int CompressPopupItem::handle_event()
{
	get_popup_menu()->set_text(get_text());
	get_popup_menu()->handle_event();
}
