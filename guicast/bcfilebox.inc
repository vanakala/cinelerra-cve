
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

#ifndef BCFILEBOX_INC
#define BCFILEBOX_INC

class BC_FileBox;


// Display modes
#define FILEBOX_LIST    0
#define FILEBOX_ICONS   1


#define FILEBOX_COLUMNS 4
//#define DIRBOX_COLUMNS 2

// Column definitions
#define FILEBOX_NAME 0
#define FILEBOX_SIZE 1
#define FILEBOX_DATE 2
#define FILEBOX_EXTENSION 3

#define FILEBOX_NAME_TEXT _("File")
#define FILEBOX_SIZE_TEXT _("Size")
#define FILEBOX_DATE_TEXT _("Date")
#define FILEBOX_EXTENSION_TEXT _("Ext.")

#define FILEBOX_HISTORY_SIZE 16

#endif
