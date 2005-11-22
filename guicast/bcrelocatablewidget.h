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
