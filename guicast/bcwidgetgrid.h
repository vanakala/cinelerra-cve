
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

#ifndef BCWIDGETGRID_H
#define BCWIDGETGRID_H

#include "arraylist.h"
#include "bcrelocatablewidget.h"
#include "bctoggle.h"
#include "bctextbox.h"
#include "bcsubwindow.h"

#define BC_WG_Rows 25
#define BC_WG_Cols 10




class BC_WidgetGrid : public BC_RelocatableWidget {
public:  
//	BC_WidgetGrid();
	BC_WidgetGrid(int x, int y, int x_r, int y_b, int colgaps, int rowgaps);

	BC_RelocatableWidget * add(BC_RelocatableWidget *h, int row, int column);
	void calculate_maxs();
	void clear_widget(int row, int column);

	int  get_h();
	int  get_h_wm();
	int  get_w();
	int  get_w_wm();

	int  getw_w(int row, int column);
	int  getw_h(int row, int column);

	int  guess_x(int col);
	int  guess_y(int row);

	void move_widgets();
	void print();
	int  reposition_widget(int x, int y, int w = -1, int h = -1);

	void set_align(int r,int c,int va, int ha);
	void set_crspan(int r,int c,int cs, int rs);
	void set_minh(int c, int h);
	void set_minw(int c, int w);
	void setw_position(int row,int column,int x, int y);

	enum {
		VALIGN_TOP,
		VALIGN_CENTER,
		VALIGN_BOTTOM
	};

	enum {
		HALIGN_LEFT,
		HALIGN_CENTER,
		HALIGN_RIGHT
	};

	enum {
		BC_WT_NONE,
		BC_WT_RelocatableWidget
	};

private:
	int            widget_types[BC_WG_Rows][BC_WG_Cols];
	int            widget_valign[BC_WG_Rows][BC_WG_Cols];
	int            widget_halign[BC_WG_Rows][BC_WG_Cols];
	int            widget_colspan[BC_WG_Rows][BC_WG_Cols];
	int            widget_rowspan[BC_WG_Rows][BC_WG_Cols];

	// array of pointers:
	BC_RelocatableWidget *widget_widgs[BC_WG_Rows][BC_WG_Cols];

	int rowgaps;
	int colgaps;

	int maxw[BC_WG_Cols];
	int maxh[BC_WG_Rows];

	int minw[BC_WG_Cols];
	int minh[BC_WG_Rows];
	
	int x_l,x_r,y_t,y_b; // left, right, top,bottom margins.
};


class BC_WidgetGridList : public ArrayList<BC_WidgetGrid*>
{
public:
	BC_WidgetGridList();
	~BC_WidgetGridList();


private:
	
};

#endif
