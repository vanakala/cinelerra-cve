#ifndef BCTITLE_H
#define BCTITLE_H

#include "bcsubwindow.h"
#include "colors.h"
#include "fonts.h"

class BC_Title : public BC_SubWindow
{
public:
	BC_Title(int x, 
		int y, 
		char *text, 
		int font = MEDIUMFONT, 
		int color = BLACK, 
		int centered = 0,
		int fixed_w = 0);
	virtual ~BC_Title();
	
	int initialize();
	int resize(int w, int h);
	int reposition(int x, int y);
	int set_color(int color);
	int update(char *text);
private:
	int draw();
	int get_size(int &w, int &h);
	
	char text[BCTEXTLEN];
	int color;
	int font;
	int centered;
// Width if fixed
	int fixed_w;
};

#endif
