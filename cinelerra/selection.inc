// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2014 Einar Rünkaru <einarrunkaru@gmail dot com>

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
