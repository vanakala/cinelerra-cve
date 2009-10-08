
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

#include "clip.h"
#include "bctitle.h"
#include "bcpot.h"
#include "bcslider.h"
#include "bcwidgetgrid.h"


BC_WidgetGrid::BC_WidgetGrid(int x1, int y1, int x2, int y2,int cgs,int rgs){
	x_l = x1;  // x position
	y_t = y1;  // y position
	x_r = x2;  // right margin (used only in get_w_wm)
	y_b = y2;  // left margin (used only in get_w_wm)
	rowgaps = rgs; 
	colgaps = cgs;

	for (int r = 0; r < BC_WG_Rows; r++)
	  minh[r] = 0;

	for (int c = 0; c < BC_WG_Cols; c++) 
	  minw[c] = 0;

	for (int r = 0; r < BC_WG_Rows; r++)
		for (int c = 0; c < BC_WG_Cols; c++) {
			widget_types[r][c] = BC_WT_NONE;
			widget_valign[r][c] = VALIGN_CENTER;
			widget_halign[r][c] = HALIGN_LEFT;
			widget_colspan[r][c] = 1;
			widget_rowspan[r][c] = 1;
		}
}

BC_RelocatableWidget * BC_WidgetGrid::add(BC_RelocatableWidget *h, int row, int column) {
	widget_types[row][column] = BC_WT_RelocatableWidget;
	widget_widgs[row][column] = h;
	return(h);
}



void BC_WidgetGrid::calculate_maxs(){
	int r,c;
	for (r = 0; r < BC_WG_Rows; r++) {
		maxh[r] = minh[r];
		for (c = 0; c < BC_WG_Cols; c++) {
			if ((widget_rowspan[r][c] == 1) && (getw_h(r,c) > maxh[r]))
				maxh[r] = getw_h(r,c);
		}
	}

	for (c = 0; c < BC_WG_Cols; c++) {
		maxw[c] = minw[c];
		for (r = 0; r < BC_WG_Rows; r++) {
			if ((widget_colspan[r][c] == 1) && (getw_w(r,c) > maxw[c]))
				maxw[c] = getw_w(r,c);
		}
	}

	// Fix up for row & colspans:
	for (c = 0; c < BC_WG_Cols; c++) {
		for (r = 0; r < BC_WG_Rows; r++) {
			int c_cs = MIN(BC_WG_Cols - c + 1, widget_colspan[r][c]);
			int c_rs = MIN(BC_WG_Rows - c + 1, widget_rowspan[r][c]);

			if ((widget_colspan[r][c] > 1)) {
				int csw = 0;
				int c2;
				for (c2 = c; c2 < c + c_cs; c2++) {
					csw += maxw[c2];
				}
				if (csw < getw_w(r,c)) {
					for (c2 = c; c2 < c + c_cs; c2++) {
						maxw[c2] += (csw - getw_w(r,c))/c_cs;
					}
				}
			}

			if ((widget_rowspan[r][c] > 1)) {
				int csh = 0;
				int r2;
				for (r2 = c; r2 < r + c_rs; r2++) {
					csh += maxh[r2];
				}
				if (csh < getw_h(r,c)) {
					for (r2 = c; r2 < r + c_rs; r2++) {
						maxh[r2] += (csh - getw_h(r,c))/c_rs;
					}
				}
			}
		}
	}
}

void BC_WidgetGrid::clear_widget(int row, int column){
	widget_types[row][column] = BC_WT_NONE;
}

int BC_WidgetGrid::get_h(){
	calculate_maxs();
	int y = 0;
	for (int i = 0; i < BC_WG_Rows; i++)
		if (maxh[i] > 0)
			y += maxh[i] + rowgaps;
	return (y);
}

int BC_WidgetGrid::get_h_wm(){
	return (y_t + get_h() + y_b);
}

int BC_WidgetGrid::get_w(){
	calculate_maxs();
	int x = 0;
	for (int i = 0; i < BC_WG_Cols; i++)
		if (maxw[i] > 0)
			x += maxw[i] + colgaps;
	return (x);
}

int BC_WidgetGrid::get_w_wm(){
	return (x_l + get_w() + x_r);
}

int BC_WidgetGrid::getw_h(int row, int column) {
	switch (widget_types[row][column]) {
	case BC_WT_NONE:
		return(0);
	case BC_WT_RelocatableWidget: 
		return(widget_widgs[row][column]->get_h());
	}
}

int BC_WidgetGrid::getw_w(int row, int column) {
	switch (widget_types[row][column]) {
	case BC_WT_NONE:
		return(0);
	case BC_WT_RelocatableWidget: 
		return(widget_widgs[row][column]->get_w());
	}
}

int BC_WidgetGrid::guess_x(int colno){
	calculate_maxs();
	int x = x_l;
	for (int i = 0; i < colno; i++)
		x += maxw[i] + colgaps;
	return (x);
}

int BC_WidgetGrid::guess_y(int colno){
	calculate_maxs();
	int y = y_t;
	for (int i = 0; i < colno; i++)
		y += maxh[i] + rowgaps;
	return (y);
}

void BC_WidgetGrid::move_widgets(){
	calculate_maxs();
	int r,c,x,y;
	int xn,yn;
	y = y_t;
	for (r = 0; r < BC_WG_Rows; r++) {
		x = x_l;
		for (c = 0; c < BC_WG_Cols; c++) {
			switch (widget_valign[r][c]){
			case VALIGN_TOP:
				yn = y;
				break;
			case VALIGN_CENTER:
				yn = y + (maxh[r] - getw_h(r,c))/2;
				break;
			case VALIGN_BOTTOM:
				yn = y + (maxh[r] - getw_h(r,c));
				break;
			}
				
			switch (widget_halign[r][c]){
			case HALIGN_LEFT:
				xn = x;
				break;
			case HALIGN_CENTER:
				xn = x + (maxw[c] - getw_w(r,c))/2;
				break;
			case HALIGN_RIGHT:
				xn = x + (maxw[c] - getw_w(r,c));
				break;
			}
			setw_position(r,c,xn,yn); // + (maxh[r] - getw_h(r,c))/2);
			x += colgaps + maxw[c];
		}
		y += rowgaps + maxh[r];
	}
}

void BC_WidgetGrid::print(){
	printf("\nWidget Grid: Widths: x_l=%d y_t=%d x_r=%d y_b=%d\n",x_l,y_t,x_r,y_b);
	calculate_maxs();
	for (int r = 0; r < BC_WG_Rows; r++) {
		for (int c = 0; c < BC_WG_Cols; c++) {
			printf("%d,%d\t",getw_w(r,c),getw_h(r,c));
		}
		printf("MAX: %d\n",maxh[r]);
	}
	printf("---------------------------------------------\n");
	for (int c = 0; c < BC_WG_Cols; c++) 
		printf("%d\t",maxw[c]);
	printf("\n\n");

}

int BC_WidgetGrid::reposition_widget(int x, int y, int w1, int h){
	x_l = x;
	y_t = y;
	move_widgets();
	return(0);
}

void BC_WidgetGrid::set_align(int r,int c,int va, int ha){
	widget_valign[r][c] = va;
	widget_halign[r][c] = ha;
}

void BC_WidgetGrid::set_crspan(int r,int c,int cs, int rs){
	widget_colspan[r][c] = cs;
	widget_rowspan[r][c] = rs;
}

void BC_WidgetGrid::set_minw(int c,int w){
	minw[c] = w;
}

void BC_WidgetGrid::set_minh(int c,int h){
	minh[c] = h;
}

void BC_WidgetGrid::setw_position(int row,int column,int x, int y) {
	switch (widget_types[row][column]) {
	case BC_WT_NONE:
		break;
	case BC_WT_RelocatableWidget: 
		widget_widgs[row][column]->reposition_widget(x,y);
		break;
	}
}


BC_WidgetGridList::BC_WidgetGridList()
 : ArrayList<BC_WidgetGrid*>()
{
}

BC_WidgetGridList::~BC_WidgetGridList()
{
}

