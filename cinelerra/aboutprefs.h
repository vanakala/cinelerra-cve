#ifndef ABOUTPREFS_H
#define ABOUTPREFS_H

#include "preferencesthread.h"

class AboutPrefs : public PreferencesDialog
{
public:
	AboutPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~AboutPrefs();
	


	int create_objects();
};


#endif
