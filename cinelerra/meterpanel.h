#ifndef METERPANEL_H
#define METERPANEL_H

#include "guicast.h"
#include "mwindow.inc"

class MeterReset;
class MeterMeter;

class MeterPanel
{
public:
	MeterPanel(MWindow *mwindow, 
		BC_WindowBase *subwindow, 
		int x, 
		int y, 
		int h,
		int meter_count,
		int use_meters,
		int use_recording = 0);
	~MeterPanel();

	int create_objects();
	int set_meters(int meter_count, int use_meters);
	static int get_meters_width(int meter_count, int use_meters);
	void reposition_window(int x, int y, int h);
	int get_reset_x();
	int get_reset_y();
	int get_meter_h();
	int get_meter_w(int number);
	void update(double *levels);
	void stop_meters();
	void change_format(int mode, int min, int max);
	virtual int change_status_event();
	void reset_meters();

	MWindow *mwindow;
	BC_WindowBase *subwindow;
	ArrayList<MeterMeter*> meters;
	MeterReset *reset;
	int meter_count;
	int use_meters;
	int x, y, h;
	int use_recording;
};


class MeterReset : public BC_Button
{
public:
	MeterReset(MWindow *mwindow, MeterPanel *panel, int x, int y);
	~MeterReset();
	int handle_event();
	MWindow *mwindow;
	MeterPanel *panel;
};

class MeterShow : public BC_Toggle
{
public:
	MeterShow(MWindow *mwindow, MeterPanel *panel, int x, int y);
	~MeterShow();
	int handle_event();
	MWindow *mwindow;
	MeterPanel *panel;
};

class MeterMeter : public BC_Meter
{
public:
	MeterMeter(MWindow *mwindow, MeterPanel *panel, int x, int y, int h, int titles);
	~MeterMeter();
	
	int button_press_event();
	
	MWindow *mwindow;
	MeterPanel *panel;
};

#endif
