#ifndef APATCHGUI_H
#define APATCHGUI_H

#include "apatchgui.inc"
#include "atrack.inc"
#include "floatauto.inc"
#include "guicast.h"
#include "panauto.inc"
#include "patchgui.h"

class AFadePatch;
class APanPatch;
class AMeterPatch;

class APatchGUI : public PatchGUI
{
public:
	APatchGUI(MWindow *mwindow, PatchBay *patchbay, ATrack *track, int x, int y);
	~APatchGUI();

	int create_objects();
	int reposition(int x, int y);
	int update(int x, int y);
	void synchronize_fade(float value_change);

	ATrack *atrack;
	AFadePatch *fade;
	APanPatch *pan;
	AMeterPatch *meter;
};

class AFadePatch : public BC_FSlider
{
public:
	AFadePatch(MWindow *mwindow, APatchGUI *patch, int x, int y, int w);
	static FloatAuto* get_keyframe(MWindow *mwindow, APatchGUI *patch);
	int handle_event();
	float update_edl();
	MWindow *mwindow;
	APatchGUI *patch;
};

class APanPatch : public BC_Pan
{
public:
	APanPatch(MWindow *mwindow, APatchGUI *patch, int x, int y);
	static PanAuto* get_keyframe(MWindow *mwindow, APatchGUI *patch);
	int handle_event();
	MWindow *mwindow;
	APatchGUI *patch;
};

class AMeterPatch : public BC_Meter
{
public:
	AMeterPatch(MWindow *mwindow, APatchGUI *patch, int x, int y);
	int button_press_event();
	MWindow *mwindow;
	APatchGUI *patch;
};


#endif
