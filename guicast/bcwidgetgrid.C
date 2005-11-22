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
		for (int c = 0; c < BC_WG_Cols; c++) {
			widget_types[r][c] = BC_WT_NONE;
			widget_valign[r][c] = VALIGN_CENTER;
			widget_halign[r][c] = HALIGN_LEFT;
		}
}

#define ADD_WIDGFUNCTION(type, name) \
 type * BC_WidgetGrid::name(type *bth, int row, int column) {  \
	widget_types[row][column] = BC_WT_RelocatableWidget;   \
	widget_widgs[row][column] = bth;                       \
	return(bth);                                           \
}

ADD_WIDGFUNCTION(BC_CheckBox          , add_checkbox    )
ADD_WIDGFUNCTION(BC_Pot               , add_pot         )
ADD_WIDGFUNCTION(BC_Radial            , add_radial      )
ADD_WIDGFUNCTION(BC_Slider            , add_slider      )
ADD_WIDGFUNCTION(BC_SubWindow         , add_subwindow   )
ADD_WIDGFUNCTION(BC_Title             , add_title       )

ADD_WIDGFUNCTION(BC_RelocatableWidget , add_widget      )
ADD_WIDGFUNCTION(BC_WidgetGrid        , add_widgetgrid  )

BC_TumbleTextBox * BC_WidgetGrid::add_tumbletextbox(BC_TumbleTextBox *bth, int row, int column) {
	widget_types[row][column] = BC_WT_TumbleTextBox;
	widget_ttbxs[row][column] = bth;
	return(bth);
}


int BC_WidgetGrid::getw_h(int row, int column) {
	switch (widget_types[row][column]) {
	case BC_WT_NONE:
		return(0);
	case BC_WT_TumbleTextBox: 
		return(widget_ttbxs[row][column]->get_h());
	case BC_WT_RelocatableWidget: 
		return(widget_widgs[row][column]->get_h());
	}
}

int BC_WidgetGrid::getw_w(int row, int column) {
	switch (widget_types[row][column]) {
	case BC_WT_NONE:
		return(0);
	case BC_WT_TumbleTextBox:
		return(widget_ttbxs[row][column]->get_w());
	case BC_WT_RelocatableWidget: 
		return(widget_widgs[row][column]->get_w());
	}
}

void BC_WidgetGrid::setw_position(int row,int column,int x, int y) {
	switch (widget_types[row][column]) {
	case BC_WT_NONE:
		break;
 	case BC_WT_TumbleTextBox:
		widget_ttbxs[row][column]->reposition_window(x,y);
		break;
	case BC_WT_RelocatableWidget: 
		widget_widgs[row][column]->reposition_widget(x,y);
		break;
	}
}

void BC_WidgetGrid::clear_widget(int row, int column){
	widget_types[row][column] = BC_WT_NONE;
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

int BC_WidgetGrid::get_w_wm(){
	return (x_l + get_w() + x_r);
}


int BC_WidgetGrid::get_w(){
	calculate_maxs();
	int x = 0;
	for (int i = 0; i < BC_WG_Cols; i++)
		if (maxw[i] > 0)
			x += maxw[i] + colgaps;
	return (x);
}

int BC_WidgetGrid::get_h_wm(){
	return (y_t + get_h() + y_b);
}

int BC_WidgetGrid::get_h(){
	calculate_maxs();
	int y = 0;
	for (int i = 0; i < BC_WG_Rows; i++)
		if (maxh[i] > 0)
			y += maxh[i] + rowgaps;
	return (y);
}

void BC_WidgetGrid::set_align(int r,int c,int va, int ha){
	widget_valign[r][c] = va;
	widget_halign[r][c] = ha;
}

int BC_WidgetGrid::reposition_widget(int x, int y){
	x_l = x;
	y_t = y;
	move_widgets();
	return(1);
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

void BC_WidgetGrid::calculate_maxs(){
	int r,c;
	for (r = 0; r < BC_WG_Rows; r++) {
		maxh[r] = 0;
		for (c = 0; c < BC_WG_Cols; c++) {
			if (getw_h(r,c) > maxh[r])
				maxh[r] = getw_h(r,c);
		}
	}
	for (c = 0; c < BC_WG_Cols; c++) {
		maxw[c] = 0;
		for (r = 0; r < BC_WG_Rows; r++) {
			if (getw_w(r,c) > maxw[c])
				maxw[c] = getw_w(r,c);
		}
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
