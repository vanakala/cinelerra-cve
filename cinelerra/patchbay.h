#ifndef PATCHBAY_H
#define PATCHBAY_H

#include "guicast.h"
#include "filexml.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "overlayframe.inc"
#include "patch.h"
#include "patchbay.inc"
#include "patchgui.inc"

class NudgePopupSeconds;
class NudgePopupNative;
class NudgePopup;

class PatchBay : public BC_SubWindow
{
public:
	PatchBay(MWindow *mwindow, MWindowGUI *gui);
	~PatchBay();

	int delete_all_patches();
	int create_objects();
	void resize_event();
	int button_press_event();
	int cursor_motion_event();
	BC_Pixmap* mode_to_icon(int mode);
	int icon_to_mode(BC_Pixmap *icon);

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
	BC_Pixmap *mode_icons[TRANSFER_TYPES];

	NudgePopup *nudge_popup;
};


class NudgePopup : public BC_PopupMenu
{
public:
	NudgePopup(MWindow *mwindow, PatchBay *patchbay);
	~NudgePopup();

	void create_objects();
	void activate_menu(PatchGUI *gui);

	MWindow *mwindow;
	PatchBay *patchbay;
	NudgePopupSeconds *seconds_item;
	NudgePopupNative *native_item;
};


class NudgePopupSeconds : public BC_MenuItem
{
public:
	NudgePopupSeconds(NudgePopup *popup);
	int handle_event();
	NudgePopup *popup;
};


class NudgePopupNative : public BC_MenuItem
{
public:
	NudgePopupNative(NudgePopup *popup);
	int handle_event();
	NudgePopup *popup;
};

#endif
