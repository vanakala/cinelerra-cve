
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

#ifndef FONTS_H
#define FONTS_H

#define LARGEFONT  0
#define MEDIUMFONT 2
#define SMALLFONT  1
#define MEDIUM_7SEGMENT 4

// Specialized fonts not available in every widget
#define BOLDFACE    0x8000
#define LARGEFONT_3D  (LARGEFONT | BOLDFACE)
#define MEDIUMFONT_3D  (MEDIUMFONT | BOLDFACE)
#define SMALLFONT_3D   (SMALLFONT | BOLDFACE)

#endif
