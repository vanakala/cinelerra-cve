
/*
 * CINELERRA
 * Copyright (C) 2014 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef SELECTION_INC
#define SELECTION_INC

#define SELECTION_TB_WIDTH 90

// Bits, lower 16 bits are in use by varitems
// Calculate the width of textbox
#define SELECTION_VARWIDTH  0x10000
// Show only items with respective bit set
// Item values are bits
#define SELECTION_VARBITITEMS  0x20000
// Item values are numbers 0..15
#define SELECTION_VARNUMITEMS  0x40000

class Selection;
class SampleRateSelection;
class FrameRateSelection;
class FrameSizeSelection;
class AspectRatioSelection;
class SampleBitsSelection;

#endif
