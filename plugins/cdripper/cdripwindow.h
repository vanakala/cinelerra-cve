#ifndef CDRIPWINDOW_H
#define CDRIPWINDOW_H

class CDRipWindowOK;
class CDRipWindowCancel;
class CDRipTextValue;
class CDRipWindowDevice;

#include "guicast.h"

#include "cdripper.h"

class CDRipWindow : public BC_Window
{
public:
	CDRipWindow(CDRipMain *cdripper, int x, int y);
	~CDRipWindow();

	int create_objects();

	int *output_rate;
	CDRipWindowOK *ok;
	CDRipWindowCancel *cancel;
	CDRipMain *cdripper;
	CDRipTextValue *track1, *min1, *sec1;
	CDRipTextValue *track2, *min2, *sec2;
	CDRipWindowDevice *device;
};

class CDRipTextValue : public BC_TextBox
{
public:
	CDRipTextValue(CDRipWindow *window, int *output, int x, int y, int w);
	~CDRipTextValue();
	
	int handle_event();
	int *output;
	CDRipWindow *window;
};

class CDRipWindowDevice : public BC_TextBox
{
public:
	CDRipWindowDevice(CDRipWindow *window, char *device, int x, int y, int w);
	~CDRipWindowDevice();
	
	int handle_event();
	char *device;
	CDRipWindow *window;
};

#endif
