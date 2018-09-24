
/*
 * CINELERRA
 * Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>
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
