#ifndef BCWIDGETGRID_H
#define BCWIDGETGRID_H

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

	BC_CheckBox          * add_checkbox     (BC_CheckBox          *bth, int row, int column);
	BC_Pot               * add_pot          (BC_Pot               *bth, int row, int column);
	BC_Radial            * add_radial       (BC_Radial            *bth, int row, int column);
	BC_Slider            * add_slider       (BC_Slider            *bth, int row, int column);
	BC_SubWindow         * add_subwindow    (BC_SubWindow         *bth, int row, int column);
	BC_Title             * add_title        (BC_Title             *bth, int row, int column);
	BC_TumbleTextBox     * add_tumbletextbox(BC_TumbleTextBox     *bth, int row, int column);
	BC_RelocatableWidget * add_widget       (BC_RelocatableWidget *bth, int row, int column);
	BC_WidgetGrid        * add_widgetgrid   (BC_WidgetGrid        *bth, int row, int column);

	void clear_widget(int row, int column);
	void set_align(int r,int c,int va, int ha);

	void calculate_maxs();
	void move_widgets();

	int reposition_widget(int x, int y, int w = -1, int h = -1);

	int  getw_w(int row, int column);
	int  getw_h(int row, int column);

	void setw_position(int row,int column,int x, int y);
	int  get_w();
	int  get_h();

	int  get_w_wm();
	int  get_h_wm();

	int  guess_x(int col);
	int  guess_y(int row);

	void print();

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
		BC_WT_NONE ,
		BC_WT_SubWindow,
		BC_WT_TumbleTextBox,
		BC_WT_WidgetGrid,
		BC_WT_RelocatableWidget
	};

private:
// Types of widgets:


	int            widget_types[BC_WG_Rows][BC_WG_Cols];
	int            widget_valign[BC_WG_Rows][BC_WG_Cols];
	int            widget_halign[BC_WG_Rows][BC_WG_Cols];
	// array of pointers:
	BC_TumbleTextBox  *widget_ttbxs[BC_WG_Rows][BC_WG_Cols];
	BC_RelocatableWidget *widget_widgs[BC_WG_Rows][BC_WG_Cols];

	int rowgaps;
	int colgaps;

	int maxw[BC_WG_Cols];
	int maxh[BC_WG_Rows];
	
	int x_l,x_r,y_t,y_b; // left, right, top,bottom margins.
};
#endif
