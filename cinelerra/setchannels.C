#include "keys.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mainsession.h"
#include "setchannels.h"

#include <math.h>


SetChannels::SetChannels(MWindow *mwindow)
 : BC_MenuItem("Output channels..."), Thread() 
{ this->mwindow = mwindow; }
 
int SetChannels::handle_event() 
{ 
	old_channels = new_channels = mwindow->session->audio_channels;
	for(int i = 0 ; i < MAXCHANNELS; i++)
	{
		this->achannel_positions[i] = mwindow->session->achannel_positions[i];
	}
//	mwindow->gui->disable_window();
	start(); 
}

void SetChannels::run()
{
	SetChannelsWindow window(mwindow, this);
	window.create_objects();
	int result = window.run_window();
	if(!result)
	{
		mwindow->undo->update_undo_audio("Output channels", 0);
		for(int i = 0 ; i < MAXCHANNELS; i++)
		{
			mwindow->session->achannel_positions[i] = this->achannel_positions[i];
		}
		mwindow->change_channels(old_channels, new_channels);
		//mwindow->defaults->update("OUTCHANNELS", new_channels);
		mwindow->undo->update_undo_audio();
		mwindow->draw();
		mwindow->session->changes_made = 1;
	}
//	mwindow->gui->enable_window();
}


SetChannelsWindow::SetChannelsWindow(MWindow *mwindow, SetChannels *setchannels)
 : BC_Window(PROGRAM_NAME ": Output Channels", 
 	mwindow->gui->get_abs_cursor_x(), 
	mwindow->gui->get_abs_cursor_y(), 
	340, 
	270)
{
	this->setchannels = setchannels;
}

SetChannelsWindow::~SetChannelsWindow()
{
	delete text;
	delete canvas;
}

int SetChannelsWindow::create_objects()
{
	add_subwindow(new BC_Title(5, 5, "Enter the output channels to use:"));
	add_subwindow(new BC_OKButton(10, get_h() - 40));
	add_subwindow(new BC_CancelButton(get_w() - 100, get_h() - 40));
	
	add_subwindow(new BC_Title(5, 60, "Position the channels in space:"));
	add_subwindow(canvas = new SetChannelsCanvas(setchannels, 10, 80, 150, 150));

	char string[1024];
	sprintf(string, "%d", setchannels->old_channels);
	add_subwindow(text = new SetChannelsTextBox(setchannels, canvas, string));
	
	canvas->draw();
}

SetChannelsTextBox::SetChannelsTextBox(SetChannels *setchannels, SetChannelsCanvas *canvas, char *text)
 : BC_TextBox(10, 30, 100, 1, text)
{
	this->setchannels = setchannels;
	this->canvas = canvas;
}

int SetChannelsTextBox::handle_event()
{
	int result = atol(get_text());
	if(result > 0 && result < MAXCHANNELS)
	{
		setchannels->new_channels = result;
		canvas->draw();
	}
}

SetChannelsCanvas::SetChannelsCanvas(SetChannels *setchannels, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->setchannels = setchannels;
	active_channel = -1;
	box_r = 10;
}

SetChannelsCanvas::~SetChannelsCanvas()
{
}

int SetChannelsCanvas::create_objects()
{
}

int SetChannelsCanvas::draw(int angle)
{
	clear_box(0, 0, get_w(), get_h());
	set_color(RED);
	int real_w = get_w() - box_r * 2;
	int real_h = get_h() - box_r * 2;
	int real_x = box_r;
	int real_y = box_r;
//	draw_circle(real_x, real_y, real_w, real_h);
	
	int x, y, w, h;
	char string[32];
	set_color(MEYELLOW);
	for(int i = 0; i < setchannels->new_channels; i++)
	{
		get_dimensions(setchannels->achannel_positions[i], x, y, w, h);
		draw_rectangle(x, y, w, h);
		sprintf(string, "%d", i + 1);
		draw_text(x + 2, y + box_r * 2 - 2, string);
	}
	if(angle > -1)
	{
		sprintf(string, "%d degrees", angle);
		draw_text(this->get_w() / 2 - 40, this->get_h() / 2, string);
	}
	
	flash();
}

int SetChannelsCanvas::button_press()
{
// get active channel
	for(int i = 0; i < setchannels->new_channels; i++)
	{
		int x, y, w, h;
		get_dimensions(setchannels->achannel_positions[i], x, y, w, h);
		if(get_cursor_x() > x && get_cursor_y() > y && 
			get_cursor_x() < x + w && get_cursor_y() < y + h)
		{
			active_channel = i;
			xytopol(degree_offset, get_cursor_x() - this->get_w() / 2, get_cursor_y() - this->get_h() / 2);
			degree_offset -= setchannels->achannel_positions[i];
			draw(setchannels->achannel_positions[i]);
//printf("degrees %d degree offset %d\n", setchannels->achannel_positions[i], degree_offset);
		}
	}
	return 1;
}

int SetChannelsCanvas::button_release()
{
	active_channel = -1;
	draw(-1);
}

int SetChannelsCanvas::cursor_motion()
{
	if(active_channel > -1)
	{
// get degrees of new channel
		int new_d;
		xytopol(new_d, get_cursor_x() - this->get_w() / 2, get_cursor_y() - this->get_h() / 2);
		new_d -= degree_offset;
		if(new_d < 0) new_d += 360;
		if(setchannels->achannel_positions[active_channel] != new_d)
		{
			setchannels->achannel_positions[active_channel] = new_d;
			draw(setchannels->achannel_positions[active_channel]);
		}
		return 1;
	}
	return 0;
}

int SetChannelsCanvas::get_dimensions(int channel_position, int &x, int &y, int &w, int &h)
{
	int real_w = this->get_w() - box_r * 2;
	int real_h = this->get_h() - box_r * 2;
	poltoxy(x, y, real_w / 2, channel_position);
	x += real_w / 2;
	y += real_h / 2;
	w = box_r * 2;
	h = box_r * 2;
}


int SetChannelsCanvas::poltoxy(int &x, int &y, int r, int d)
{
	float radians = (float)(d - 90) / 360 * 2 * M_PI;
	y = (int)(sin(radians) * r);
	x = (int)(cos(radians) * r);
}

int SetChannelsCanvas::xytopol(int &d, int x, int y)
{
	float x1, y1;
	float angle;
	y *= -1;
	x *= -1;
	
	if(x < 0 && y > 0){
		x1 = y;
		y1 = x;
	}else{
		x1 = x;
		y1 = y;
	}

	if(!y || !x){
		if(x < 0) angle = .5;
		else
		if(x > 0) angle = 1.5;
		else
		if(y < 0) angle = 1;
		else
		if(y > 0) angle = 0;
	}else{
		angle = atan(y1 / x1);
		angle /= M_PI;
	}

// fix angle

	if(x < 0 && y < 0){
		angle += .5;
	}else
	if(x > 0 && y > 0){
		angle += 1.5;
	}else
	if(x > 0 && y < 0){
		angle += 1.5;
	}

	if(x < 0 && y > 0) angle = -angle;
	angle /= 2;
	angle *= 360;
	d =  (int)angle;
}


