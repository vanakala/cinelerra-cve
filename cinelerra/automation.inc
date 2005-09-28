#ifndef AUTOMATION_INC
#define AUTOMATION_INC

class Automation;

// These must match the following arrays:
// xml_titles in autoconf.C and automation.C.
// auto_colors in trackcanvas.C
// auto_operations in trackcanvas.C
// auto_pixmaps in trackcanvas.C
// auto_text in gwindowgui.C
enum
{
	AUTOMATION_MUTE,
	AUTOMATION_CAMERA_X,
	AUTOMATION_CAMERA_Y,
	AUTOMATION_CAMERA_Z,
	AUTOMATION_PROJECTOR_X,
	AUTOMATION_PROJECTOR_Y,
	AUTOMATION_PROJECTOR_Z,
	AUTOMATION_FADE,
	AUTOMATION_PAN,
	AUTOMATION_MODE,
	AUTOMATION_MASK,
//	AUTOMATION_NUDGE,
	AUTOMATION_TOTAL
};


// Automation types
enum
{
	AUTOMATION_TYPE_FLOAT,
	AUTOMATION_TYPE_MASK,
	AUTOMATION_TYPE_INT,
	AUTOMATION_TYPE_PAN
};


#endif
