#include "guicast.h"
#include "motion.inc"

class MasterLayer : public BC_PopupMenu
{
public:
	MasterLayer(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain *plugin;
	MotionWindow *gui;
};

class Mode1 : public BC_PopupMenu
{
public:
	Mode1(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain *plugin;
	MotionWindow *gui;
};

class Mode2 : public BC_PopupMenu
{
public:
	Mode2(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	MotionMain *plugin;
	MotionWindow *gui;
};

class Mode3 : public BC_PopupMenu
{
public:
	Mode3(MotionMain *plugin, MotionWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(MotionWindow *gui);
	static void from_text(int *horizontal_only, int *vertical_only, char *text);
	static char* to_text(int horizontal_only, int vertical_only);
	MotionMain *plugin;
	MotionWindow *gui;
};


class TrackSingleFrame : public BC_Radial
{
public:
	TrackSingleFrame(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class TrackFrameNumber : public BC_TextBox
{
public:
	TrackFrameNumber(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class TrackPreviousFrame : public BC_Radial
{
public:
	TrackPreviousFrame(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class PreviousFrameSameBlock : public BC_Radial
{
public:
	PreviousFrameSameBlock(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class GlobalRange : public BC_IPot
{
public:
	GlobalRange(MotionMain *plugin, 
		int x, 
		int y,
		int *value);
	int handle_event();
	MotionMain *plugin;
	int *value;
};

class RotationRange : public BC_IPot
{
public:
	RotationRange(MotionMain *plugin, 
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
};

class BlockSize : public BC_IPot
{
public:
	BlockSize(MotionMain *plugin, 
		int x, 
		int y,
		int *value);
	int handle_event();
	MotionMain *plugin;
	int *value;
};

class MotionBlockX : public BC_FPot
{
public:
	MotionBlockX(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionBlockY : public BC_FPot
{
public:
	MotionBlockY(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionBlockXText : public BC_TextBox
{
public:
	MotionBlockXText(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionBlockYText : public BC_TextBox
{
public:
	MotionBlockYText(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class GlobalSearchPositions : public BC_PopupMenu
{
public:
	GlobalSearchPositions(MotionMain *plugin, 
		int x, 
		int y,
		int w);
	void create_objects();
	int handle_event();
	MotionMain *plugin;
};

class RotationSearchPositions : public BC_PopupMenu
{
public:
	RotationSearchPositions(MotionMain *plugin, 
		int x, 
		int y,
		int w);
	void create_objects();
	int handle_event();
	MotionMain *plugin;
};

class MotionMagnitude : public BC_IPot
{
public:
	MotionMagnitude(MotionMain *plugin, 
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
};

class MotionReturnSpeed : public BC_IPot
{
public:
	MotionReturnSpeed(MotionMain *plugin, 
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
};



class MotionDrawVectors : public BC_CheckBox
{
public:
	MotionDrawVectors(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionMain *plugin;
	MotionWindow *gui;
};

class AddTrackedFrameOffset : public BC_CheckBox
{
public:
	AddTrackedFrameOffset(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionGlobal : public BC_CheckBox
{
public:
	MotionGlobal(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};

class MotionRotate : public BC_CheckBox
{
public:
	MotionRotate(MotionMain *plugin, 
		MotionWindow *gui,
		int x, 
		int y);
	int handle_event();
	MotionWindow *gui;
	MotionMain *plugin;
};



class MotionWindow : public BC_Window
{
public:
	MotionWindow(MotionMain *plugin, int x, int y);
	~MotionWindow();

	int create_objects();
	int close_event();
	void update_mode();
	char* get_radius_title();

	GlobalRange *global_range_w;
	GlobalRange *global_range_h;
	RotationRange *rotation_range;
	BlockSize *global_block_w;
	BlockSize *global_block_h;
	BlockSize *rotation_block_w;
	BlockSize *rotation_block_h;
	MotionBlockX *block_x;
	MotionBlockY *block_y;
	MotionBlockXText *block_x_text;
	MotionBlockYText *block_y_text;
	GlobalSearchPositions *global_search_positions;
	RotationSearchPositions *rotation_search_positions;
	MotionMagnitude *magnitude;
	MotionReturnSpeed *return_speed;
	Mode1 *mode1;
	MotionDrawVectors *vectors;
	MotionGlobal *global;
	MotionRotate *rotate;
	AddTrackedFrameOffset *addtrackedframeoffset;
	TrackSingleFrame *track_single;
	TrackFrameNumber *track_frame_number;
	TrackPreviousFrame *track_previous;
	PreviousFrameSameBlock *previous_same;
	MasterLayer *master_layer;
	Mode2 *mode2;
	Mode3 *mode3;

	MotionMain *plugin;
};



PLUGIN_THREAD_HEADER(MotionMain, MotionThread, MotionWindow)





