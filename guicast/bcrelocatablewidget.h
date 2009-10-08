
/*
 * CINELERRA
 * Copyright (C) 2005 Pierre Dumuid
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

#ifndef BCRELOCATABLEWIDGET_H
#define BCRELOCATABLEWIDGET_H

class BC_RelocatableWidget
{
public:
	BC_RelocatableWidget();
	virtual int reposition_widget(int x, int y, int w = -1, int h = -1) { return -1; };
	virtual int get_w() {return -1; };
	virtual int get_h() {return -1; };
};

#endif
