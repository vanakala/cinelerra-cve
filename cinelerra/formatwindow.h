#ifndef FORMATWINDOW_H
#define FORMATWINDOW_H


class FormatHILO;
class FormatLOHI;



// Gets the parameters for all the built-in compression formats.
#include "assets.inc"
#include "guicast.h"
#include "bitspopup.h"
#include "compresspopup.h"

class FormatAWindow : public BC_Window
{
public:
	FormatAWindow(Asset *asset, int *dither);
	~FormatAWindow();
	int create_objects();
	int close_event();

	Asset *asset;
	int *dither;
	FormatHILO *hilo_button;
	FormatLOHI *lohi_button;
};

class FormatVWindow : public BC_Window
{
public:
	FormatVWindow(Asset *asset, int recording);
	~FormatVWindow();
	int create_objects();
	int close_event();

	Asset *asset;
	int recording;
};


class FormatCompress : public CompressPopup
{
public:
	FormatCompress(int x, int y, int recording, Asset *asset, char *default_);
	~FormatCompress();

	int handle_event();
	Asset *asset;
};

class FormatQuality : public BC_ISlider
{
public:
	FormatQuality(int x, int y, Asset *asset, int default_);
	~FormatQuality();
	int handle_event();
	Asset *asset;
};

class FormatBits : public BitsPopup
{
public:
	FormatBits(int x, int y, Asset *asset);
	~FormatBits();
	
	int handle_event();
	Asset *asset;
};

class FormatDither : public BC_CheckBox
{
public:
	FormatDither(int x, int y, int *dither);
	~FormatDither();

	int handle_event();
	int *dither;
};

class FormatSigned : public BC_CheckBox
{
public:
	FormatSigned(int x, int y, Asset *asset);
	~FormatSigned();
	
	int handle_event();
	Asset *asset;
};

class FormatHILO : public BC_Radial
{
public:
	FormatHILO(int x, int y, Asset *asset);
	~FormatHILO();
	
	int handle_event();
	FormatLOHI *lohi;
	Asset *asset;
};

class FormatLOHI : public BC_Radial
{
public:
	FormatLOHI(int x, int y, FormatHILO *hilo, Asset *asset);
	~FormatLOHI();

	int handle_event();
	FormatHILO *hilo;
	Asset *asset;
};



#endif
