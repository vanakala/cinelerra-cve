#ifndef NEWORAPPEND_H
#define NEWORAPPEND_H

#include "guicast.h"
#include "confirmsave.inc"
#include "file.inc"
#include "filexml.inc"

class NewOrAppendNewButton;
class NewOrAppendCancelButton;
class NewOrAppendAppendButton;

class NewOrAppend
{
public:
	NewOrAppend(MWindow *mwindow);
	~NewOrAppend();
	
	int test_file(Asset *asset, FileXML *script = 0);
	// 2 append
	// 1 cancel
	// 0 replace
	MWindow *mwindow;
};

class NewOrAppendWindow : public BC_Window
{
public:
	NewOrAppendWindow(MWindow *mwindow, Asset *asset, int confidence);
	~NewOrAppendWindow();
	
	int confidence;
	int create_objects();
	Asset *asset;
	NewOrAppendCancelButton *cancel;
	NewOrAppendNewButton *ok;
	NewOrAppendAppendButton *append;
	MWindow *mwindow;
};

class NewOrAppendNewButton : public BC_Button
{
public:
	NewOrAppendNewButton(NewOrAppendWindow *nwindow);
	
	int handle_event();
	int keypress_event();
	
	NewOrAppendWindow *nwindow;
};

class NewOrAppendAppendButton : public BC_Button
{
public:
	NewOrAppendAppendButton(NewOrAppendWindow *nwindow);
	
	int handle_event();
	int keypress_event();
	
	NewOrAppendWindow *nwindow;
};

class NewOrAppendCancelButton : public BC_Button
{
public:
	NewOrAppendCancelButton(NewOrAppendWindow *nwindow);
	
	int handle_event();
	int keypress_event();
	
	NewOrAppendWindow *nwindow;
};

#endif
