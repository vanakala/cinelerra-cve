#ifndef PATCH_H
#define PATCH_H

class PlayPatchOld;
class RecordPatchOld;
class TitlePatchOld;
class AutoPatchOld;
class DrawPatchOld;

#include "guicast.h"
#include "filexml.inc"
#include "mwindow.inc"
#include "module.inc"
#include "patch.inc"
#include "patchbay.inc"


// coordinates for widgets

#define PATCH_ROW2 23
#define PATCH_TITLE 3
#define PATCH_PLAY 7
#define PATCH_REC 27
#define PATCH_AUTO 47
#define PATCH_AUTO_TITLE 67
#define PATCH_DRAW 72
#define PATCH_DRAW_TITLE 97


class Patch : public ListItem<Patch>
{
public:
	Patch() {};
	Patch(MWindow *mwindow, PatchBay *patchbay, int data_type);
	~Patch();

	int save(FileXML *xml);
	int load(FileXML *xml);

	int create_objects(char *text, int pixel); // linked list doesn't allow parameters in append()
	int set_pixel(int pixel);
	int set_title(char *new_title);
	int flip_vertical();
	int pixelmovement(int distance);

	int pixel;      // distance from top of track window
	int data_type;
	int record;
	int play;
	int automate;
	int draw;
	char title[1024];















	PatchBay *patches;
	MWindow *mwindow;

	RecordPatchOld *recordpatch;
	PlayPatchOld *playpatch;
	TitlePatchOld *title_text;
	AutoPatchOld *autopatch;
	DrawPatchOld *drawpatch;
	Module* get_module();    // return module of corresponding patch
};

class PlayPatchOld : public BC_Toggle
{
public:
	PlayPatchOld(MWindow *mwindow, Patch *patch, int x, int y);
	int handle_event();
	int cursor_moved_over();
	int button_release();
	PatchBay *patches;
	Patch *patch;
};

class RecordPatchOld : public BC_Toggle
{
public:
	RecordPatchOld(MWindow *mwindow, Patch *patch, int x, int y);
	int handle_event();
	int cursor_moved_over();
	int button_release();
	Patch *patch;
	PatchBay *patches;
};

class TitlePatchOld : public BC_TextBox
{
public:
	TitlePatchOld(MWindow *mwindow, Patch *patch, char *text, int x, int y);
	int handle_event();
	Patch *patch;
	PatchBay *patches;
	Module *module;
};

class AutoPatchOld : public BC_Toggle
{
public:
	AutoPatchOld(MWindow *mwindow, Patch *patch, int x, int y);
	int handle_event();
	int cursor_moved_over();
	int button_release();
	PatchBay *patches;
	Patch *patch;
};

class DrawPatchOld : public BC_Toggle
{
public:
	DrawPatchOld(MWindow *mwindow, Patch *patch, int x, int y);
	int handle_event();
	int cursor_moved_over();
	PatchBay *patches;
	Patch *patch;
};

#endif
