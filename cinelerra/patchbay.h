#ifndef PATCHBAY_H
#define PATCHBAY_H

#include "guicast.h"
#include "filexml.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "patch.h"
#include "patchbay.inc"
#include "patchgui.inc"



class PatchBay
 : public BC_SubWindow
{
public:
	PatchBay(MWindow *mwindow, MWindowGUI *gui);
	~PatchBay();

	int delete_all_patches();
	int create_objects();
	void resize_event();
	int button_press_event();
	int cursor_motion_event();

// Synchronize with Master EDL
	int update();
	void update_meters(ArrayList<double> *module_levels);
	void stop_meters();
	void synchronize_faders(float value, int data_type, Track *skip);
	void change_meter_format(int mode, float min);
	void reset_meters();

	ArrayList<PatchGUI*> patches;






// =========================================== drawing

	int resize_event(int top, int bottom);


	MWindow *mwindow;
	MWindowGUI *gui;

	int button_down, new_status, drag_operation, reconfigure_trigger;
};

#endif
