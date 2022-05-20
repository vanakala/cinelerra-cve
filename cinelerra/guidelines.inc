// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef GUIDELINES_INC
#define GUIDELINES_INC

#define GUIDELINE_NONE        0
#define GUIDELINE_LINE        1 // coordinates of endpoints
#define GUIDELINE_RECTANGLE   2 // coordinate of upper-left corner, width, height
#define GUIDELINE_BOX         3 // coordinate of upper-left corner, width, height
#define GUIDELINE_DISC        4 // coordinate of upper-left corner, width, height
#define GUIDELINE_CIRCLE      5 // coordinate of upper-left corner, width, height
#define GUIDELINE_PIXEL       6 // coordinates
#define GUIDELINE_VFRAME      7 // no parameters

class GuideLines;
class GuideFrame;

#endif
