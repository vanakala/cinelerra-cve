#ifndef BCPROGRESSBOX_H
#define BCPROGRESSBOX_H

#include "bcprogress.inc"
#include "bcprogressbox.inc"
#include "bctitle.inc"
#include "bcwindow.h"
#include "thread.h"

class BC_ProgressBox : public Thread
{
public:
	BC_ProgressBox(int x, int y, char *text, long length);
	virtual ~BC_ProgressBox();
	
	friend class BC_ProgressWindow;

	void run();
	int update(long position);    // return 1 if cancelled
	int update_title(char *title);
	int update_length(long length);
	int is_cancelled();      // return 1 if cancelled
	int stop_progress();
	void lock_window();
	void unlock_window();

private:
	BC_ProgressWindow *pwindow;
	char *display;
	char *text;
	int cancelled;
	long length;
};


class BC_ProgressWindow : public BC_Window
{
public:
	BC_ProgressWindow(int x, int y);
	virtual ~BC_ProgressWindow();

	int create_objects(char *text, long length);

	char *text;
	BC_ProgressBar *bar;
	BC_Title *caption;
};

#endif
