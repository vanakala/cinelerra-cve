#ifndef ASSETEDIT_H
#define ASSETEDIT_H

#include "asset.inc"
#include "awindow.inc"
#include "guicast.h"
#include "bitspopup.inc"
#include "browsebutton.h"
#include "formatpopup.h"
#include "mwindow.h"
#include "thread.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

class AssetEditByteOrderHILO;
class AssetEditByteOrderLOHI;
class AssetEditPath;
class AssetEditPathText;
class AssetEditWindow;

class AssetEdit : public Thread
{
public:
	AssetEdit(MWindow *mwindow);
	~AssetEdit();
	
	void edit_asset(Asset *asset);
	int set_asset(Asset *asset);
	void run();

	Asset *asset, *new_asset;
	MWindow *mwindow;
	AssetEditWindow *window;
};



// Pcm is the only format users should be able to fix.
// All other formats display information about the file in read-only.

class AssetEditWindow : public BC_Window
{
public:
	AssetEditWindow(MWindow *mwindow, AssetEdit *asset_edit);
	~AssetEditWindow();

	int create_objects();
	Asset *asset;
	AssetEditPathText *path_text;
	AssetEditPath *path_button;
	AssetEditByteOrderHILO *hilo;
	AssetEditByteOrderLOHI *lohi;
	BitsPopup *bitspopup;
	int allow_edits;
	MWindow *mwindow;
	AssetEdit *asset_edit;
};


class AssetEditPath : public BrowseButton
{
public:
	AssetEditPath(MWindow *mwindow, AssetEditWindow *fwindow, BC_TextBox *textbox, int y, char *text, char *window_title = "2000: Path", char *window_caption = _("Select a file"));
	~AssetEditPath();
	
	AssetEditWindow *fwindow;
};


class AssetEditPathText : public BC_TextBox
{
public:
	AssetEditPathText(AssetEditWindow *fwindow, int y);
	~AssetEditPathText();
	int handle_event();

	AssetEditWindow *fwindow;
};



class AssetEditFormat : public FormatPopup
{
public:
	AssetEditFormat(AssetEditWindow *fwindow, char* default_, int y);
	~AssetEditFormat();
	
	int handle_event();
	AssetEditWindow *fwindow;
};


class AssetEditChannels : public BC_TumbleTextBox
{
public:
	AssetEditChannels(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditRate : public BC_TextBox
{
public:
	AssetEditRate(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditFRate : public BC_TextBox
{
public:
	AssetEditFRate(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditHeader : public BC_TextBox
{
public:
	AssetEditHeader(AssetEditWindow *fwindow, char *text, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditByteOrderLOHI : public BC_Radial
{
public:
	AssetEditByteOrderLOHI(AssetEditWindow *fwindow, int value, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditByteOrderHILO : public BC_Radial
{
public:
	AssetEditByteOrderHILO(AssetEditWindow *fwindow, int value, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditSigned : public BC_CheckBox
{
public:
	AssetEditSigned(AssetEditWindow *fwindow, int value, int x, int y);
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

#endif
