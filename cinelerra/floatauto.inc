
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

#ifndef FLOATAUTO_INC
#define FLOATAUTO_INC

class FloatAuto;
// Possible policies to handle the tagents for the
// bézier curves connecting adjacent automation points
enum tgnt_mode
{
	TGNT_SMOOTH,     // tangents are coupled in order to yield a smooth curve
	TGNT_LINEAR,     // tangents always pointing directly to neighbouring automatio
	TGNT_TFREE,      // tangents on both sides coupled but editable by dragging the
	TGNT_FREE        // tangents on both sides are independent and editable via GUI
};

#endif
