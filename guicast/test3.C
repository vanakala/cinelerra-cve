#include "bcsignals.h"
#include "guicast.h"




class TestWindow : public BC_Window
{
public:
	TestWindow() : BC_Window("Test",
		0,
		0,
		320,
		240)
	{
	};

	void create_objects()
	{
		set_color(BLACK);
		set_font(LARGEFONT);
		draw_text(10, 50, "Hello world");
		flash();
		flush();
	};
};


int main()
{
	new BC_Signals;
	TestWindow window;
	window.create_objects();
	window.run_window();
}
