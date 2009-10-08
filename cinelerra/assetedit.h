
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef ASSETEDIT_H
#define ASSETEDIT_H

#include "asset.inc"
#include "awindow.inc"
#include "guicast.h"
#include "bitspopup.inc"
#include "browsebutton.h"
#include "formatpopup.h"
#include "language.h"
#include "mwindow.h"
#include "thread.h"


class AssetEditTCStartTextBox;
class AssetEditReelNumber;
class AssetEditReelName;
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
	AssetEditPath(MWindow *mwindow, 
		AssetEditWindow *fwindow, 
		BC_TextBox *textbox, 
		int y, 
		char *text, 
		char *window_title = _(PROGRAM_NAME " Path"), 
		char *window_caption = _("Select a file"));
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

class Interlaceautofix : public BC_CheckBox
{
public:
	Interlaceautofix(MWindow *mwindow,AssetEditWindow *fwindow, int x, int y);
	~Interlaceautofix();
	int handle_event();

	void showhideotherwidgets();

	AssetEditWindow* fwindow;
	MWindow *mwindow;

	BC_TextBox *ilacemode_textbox;
	BC_ListBox *ilacemode_listbox;
	BC_TextBox *ilacefixmethod_textbox;
	BC_ListBox *ilacefixmethod_listbox;
private:
  	char string[BCTEXTLEN];
};

class AssetEditILaceautofixoption : public BC_TextBox
{
public:
	AssetEditILaceautofixoption(AssetEditWindow *fwindow, char *text, int thedefault, int x, int y, int w);

	int handle_event();
	int thedefault;
	AssetEditWindow *fwindow;
};

class AssetEditILacemode : public BC_TextBox
{
public:
	AssetEditILacemode(AssetEditWindow *fwindow, char *text, int thedefault, int x, int y, int w);
	int handle_event();
	int thedefault;	
	AssetEditWindow *fwindow;
};

class AssetEditInterlacemodePulldown : public BC_ListBox
{
public:
	AssetEditInterlacemodePulldown(MWindow *mwindow, 
				BC_TextBox *output_text, 
				int *output_value,
				ArrayList<BC_ListBoxItem*> *data,
				Interlaceautofix *fixoption_chkbox,
				int x,
				int y);
	int handle_event();
	char* interlacemode_to_text();
	MWindow *mwindow;
	BC_TextBox *output_text;
	int *output_value;
	Interlaceautofix *fixoption_chkbox;
private:
  	char string[BCTEXTLEN];
};

class AssetEditILacefixmethod : public BC_TextBox
{
public:
	AssetEditILacefixmethod(AssetEditWindow *fwindow, char *text, int thedefault, int x, int y, int w);
	
	int handle_event();
	int thedefault;
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

class AssetEditReelName : public BC_TextBox
{
public:
	AssetEditReelName(AssetEditWindow *fwindow, int x, int y);
	~AssetEditReelName();

	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditReelNumber : public BC_TextBox
{
public:
	AssetEditReelNumber(AssetEditWindow *fwindow, int x, int y);
	~AssetEditReelNumber();
	
	int handle_event();
	
	AssetEditWindow *fwindow;
};

class AssetEditTCStartTextBox : public BC_TextBox
{
public:
	AssetEditTCStartTextBox(AssetEditWindow *fwindow, int value, int x, int y, int multiplier);
	~AssetEditTCStartTextBox();
	int handle_event();
	
	AssetEditWindow *fwindow;
// Multiplier is the # of frames for whatever unit of time this is.
// fps dependent, and unit dependent
	int multiplier;
	int previous;
};
#endif
