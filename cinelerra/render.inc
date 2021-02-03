// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RENDER_INC
#define RENDER_INC

class PackageRenderer;
class Render;
class RenderItem;
class RenderJob;
class RenderWindow;

// Internal rendering strategies (bits)
#define RENDER_FILE_PER_LABEL   2
#define RENDER_BRENDER          4
#define RENDER_FARM             0x80

#define RANGE_PROJECT           1
#define RANGE_SELECTION         2
#define RANGE_INOUT             3
#define RANGE_SINGLEFRAME       4

#endif
