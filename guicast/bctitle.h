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
		int color = -1, 
		int centered = 0,
		int fixed_w = 0);
	virtual ~BC_Title();
	
	int initialize();
	static int calculate_w(BC_WindowBase *gui, char *text, int font = MEDIUMFONT);
	static int calculate_h(BC_WindowBase *gui, char *text, int font = MEDIUMFONT);
	int resize(int w, int h);
	int reposition(int x, int y);
	int set_color(int color);
	int update(char *text);
	void update(float value);
	char* get_text();

private:
	int draw();
	static void get_size(BC_WindowBase *gui, int font, char *text, int fixed_w, int &w, int &h);
	
	char text[BCTEXTLEN];
	int color;
	int font;
	int centered;
// Width if fixed
	int fixed_w;
};

#endif
