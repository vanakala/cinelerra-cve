
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

#ifndef RENDER_INC
#define RENDER_INC

class PackageRenderer;
class Render;
class RenderItem;
class RenderJob;
class RenderWindow;

// Internal rendering strategies

#define SINGLE_PASS             0
#define FILE_PER_LABEL          1
#define SINGLE_PASS_FARM        2
#define FILE_PER_LABEL_FARM     3
#define BRENDER_FARM            4


#define RANGE_BACKCOMPAT	0
#define RANGE_PROJECT		1
#define RANGE_SELECTION		2
#define RANGE_INOUT		3

#endif
