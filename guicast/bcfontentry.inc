
/*
 * CINELERRA
 * Copyright (C) 2014 Einar Rünkaru <einarry at smail dot ee>
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

#ifndef BCFONTENTRY_INC
#define BCFONTENTRY_INC

class BC_FontEntry;

// Bits in BC_FontEntry::style
#define FL_WEIGHT_THIN          0x0000001
#define FL_WEIGHT_EXTRALIGHT    0x0000002
#define FL_WEIGHT_LIGHT         0x0000004
#define FL_WEIGHT_MEDIUM        0x0000008
#define FL_WEIGHT_BOOK          0x0000010
#define FL_WEIGHT_NORMAL        0x0000020
#define FL_WEIGHT_DEMIBOLD      0x0000040
#define FL_WEIGHT_BOLD          0x0000080
#define FL_WEIGHT_EXTRABOLD     0x0000100
#define FL_WEIGHT_BLACK         0x0000200
#define FL_WEIGHT_EXTRABLACK    0x0000400
#define FL_WEIGHT_MASK          0x00007ff

#define FL_SLANT_ROMAN          0x0000800
#define FL_SLANT_ITALIC         0x0001000
#define FL_SLANT_OBLIQUE        0x0002000
#define FL_SLANT_MASK           0x0003800

#define FL_WIDTH_ULTRACONDENSED 0x0004000
#define FL_WIDTH_EXTRACONDENSED 0x0008000
#define FL_WIDTH_CONDENSED      0x0010000
#define FL_WIDTH_SEMICONDENSED  0x0020000
#define FL_WIDTH_NORMAL         0x0040000
#define FL_WIDTH_SEMIEXPANDED   0x0080000
#define FL_WIDTH_EXPANDED       0x0100000
#define FL_WIDTH_EXTRAEXPANDED  0x0200000
#define FL_WIDTH_ULTRAEXPANDED  0x0400000
#define FL_WIDTH_MASK           0x07fc000

#define FL_PROPORTIONAL         0x0800000
#define FL_DUAL                 0x1000000
#define FL_MONO                 0x2000000
#define FL_CHARCELL             0x4000000
#define FL_SPACING_MASK         0x7800000

// init_fontconfig options
#define FLFC_LANGUAGE          1      // Filter language

#endif
