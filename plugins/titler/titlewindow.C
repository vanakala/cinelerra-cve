#include "bcdisplayinfo.h"
#include "titlewindow.h"

#include <string.h>

TitleThread::TitleThread(TitleMain *client)
 : Thread()
{
	this->client = client;
	set_synchronous(0);
	gui_started.lock();
	completion.lock();
}

TitleThread::~TitleThread()
{
// Window always deleted here
	delete window;
}
	
void TitleThread::run()
{
	BC_DisplayInfo info;
	window = new TitleWindow(client, 
		info.get_abs_cursor_x() - client->window_w / 2, 
		info.get_abs_cursor_y() - client->window_h / 2);
	window->create_objects();
	gui_started.unlock();
	int result = window->run_window();
	completion.unlock();
// Last command executed in thread
	if(result) client->client_side_close();
}









TitleWindow::TitleWindow(TitleMain *client, int x, int y)
 : BC_Window(client->gui_string, 
	x,
	y,
	client->window_w, 
	client->window_h, 
	100, 
	100, 
	1, 
	0,
	1)
{ 
	this->client = client; 
}

TitleWindow::~TitleWindow()
{
	sizes.remove_all_objects();
	encodings.remove_all_objects();
	delete color_thread;
	delete title_x;
	delete title_y;
}

int TitleWindow::create_objects()
{
	int x = 10, y = 10;
	
	encodings.append(new BC_ListBoxItem("ISO8859-1"));
	encodings.append(new BC_ListBoxItem("ISO8859-2"));
	encodings.append(new BC_ListBoxItem("ISO8859-3"));
	encodings.append(new BC_ListBoxItem("ISO8859-4"));
	encodings.append(new BC_ListBoxItem("ISO8859-5"));
	encodings.append(new BC_ListBoxItem("ISO8859-6"));
	encodings.append(new BC_ListBoxItem("ISO8859-7"));
	encodings.append(new BC_ListBoxItem("ISO8859-8"));
	encodings.append(new BC_ListBoxItem("ISO8859-9"));
	encodings.append(new BC_ListBoxItem("ISO8859-10"));
	encodings.append(new BC_ListBoxItem("ISO8859-11"));
	encodings.append(new BC_ListBoxItem("ISO8859-12"));
	encodings.append(new BC_ListBoxItem("ISO8859-13"));
	encodings.append(new BC_ListBoxItem("ISO8859-14"));
	encodings.append(new BC_ListBoxItem("ISO8859-15"));
	encodings.append(new BC_ListBoxItem("KOI8"));



	sizes.append(new BC_ListBoxItem("8"));
	sizes.append(new BC_ListBoxItem("9"));
	sizes.append(new BC_ListBoxItem("10"));
	sizes.append(new BC_ListBoxItem("11"));
	sizes.append(new BC_ListBoxItem("12"));
	sizes.append(new BC_ListBoxItem("13"));
	sizes.append(new BC_ListBoxItem("14"));
	sizes.append(new BC_ListBoxItem("16"));
	sizes.append(new BC_ListBoxItem("18"));
	sizes.append(new BC_ListBoxItem("20"));
	sizes.append(new BC_ListBoxItem("22"));
	sizes.append(new BC_ListBoxItem("24"));
	sizes.append(new BC_ListBoxItem("26"));
	sizes.append(new BC_ListBoxItem("28"));
	sizes.append(new BC_ListBoxItem("32"));
	sizes.append(new BC_ListBoxItem("36"));
	sizes.append(new BC_ListBoxItem("40"));
	sizes.append(new BC_ListBoxItem("48"));
	sizes.append(new BC_ListBoxItem("56"));
	sizes.append(new BC_ListBoxItem("64"));
	sizes.append(new BC_ListBoxItem("72"));
	sizes.append(new BC_ListBoxItem("100"));
	sizes.append(new BC_ListBoxItem("128"));

	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(NO_MOTION)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(BOTTOM_TO_TOP)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(TOP_TO_BOTTOM)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(RIGHT_TO_LEFT)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(LEFT_TO_RIGHT)));



// Construct font list
	for(int i = 0; i < client->fonts->total; i++)
	{
		int exists = 0;
		for(int j = 0; j < fonts.total; j++)
		{
			if(!strcasecmp(fonts.values[j]->get_text(), 
				client->fonts->values[i]->fixed_title)) 
			{
				exists = 1;
				break;
			}
		}

		if(!exists) fonts.append(new 
			BC_ListBoxItem(client->fonts->values[i]->fixed_title));
	}

// Sort font list
	int done = 0;
	while(!done)
	{
		done = 1;
		for(int i = 0; i < fonts.total - 1; i++)
		{
			if(strcmp(fonts.values[i]->get_text(), fonts.values[i + 1]->get_text()) > 0)
			{
				BC_ListBoxItem *temp = fonts.values[i + 1];
				fonts.values[i + 1] = fonts.values[i];
				fonts.values[i] = temp;
				done = 0;
			}
		}
	}	












	add_tool(font_title = new BC_Title(x, y, "Font:"));
	font = new TitleFont(client, this, x, y + 20);
	font->create_objects();
	x += 230;
	add_subwindow(font_tumbler = new TitleFontTumble(client, this, x, y + 20));
	x += 30;
	char string[BCTEXTLEN];
	add_tool(size_title = new BC_Title(x, y, "Size:"));
	sprintf(string, "%d", client->config.size);
	size = new TitleSize(client, this, x, y + 20, string);
	size->create_objects();
	x += 130;

	add_tool(style_title = new BC_Title(x, y, "Style:"));
	add_tool(italic = new TitleItalic(client, this, x, y + 20));
	add_tool(bold = new TitleBold(client, this, x, y + 50));
	x += 80;

	add_tool(justify_title = new BC_Title(x, y, "Justify:"));
	add_tool(left = new TitleLeft(client, this, x, y + 20));
	add_tool(center = new TitleCenter(client, this, x, y + 50));
	add_tool(right = new TitleRight(client, this, x, y + 80));

	x += 80;
	add_tool(top = new TitleTop(client, this, x, y + 20));
	add_tool(mid = new TitleMid(client, this, x, y + 50));
	add_tool(bottom= new TitleBottom(client, this, x, y + 80));
	


	y += 50;
	x = 10;

	add_tool(x_title = new BC_Title(x, y, "X:"));
	title_x = new TitleX(client, this, x, y + 20);
	title_x->create_objects();
	x += 90;

	add_tool(y_title = new BC_Title(x, y, "Y:"));
	title_y = new TitleY(client, this, x, y + 20);
	title_y->create_objects();
	x += 90;

	add_tool(motion_title = new BC_Title(x, y, "Motion type:"));

	motion = new TitleMotion(client, this, x, y + 20);
	motion->create_objects();
	x += 150;
	
	add_tool(loop = new TitleLoop(client, x, y + 20));
	x += 100;
	
	x = 10;
	y += 50;

	add_tool(dropshadow_title = new BC_Title(x, y, "Drop shadow:"));
	dropshadow = new TitleDropShadow(client, this, x, y + 20);
	dropshadow->create_objects();
	x += 100;

	add_tool(fadein_title = new BC_Title(x, y, "Fade in (sec):"));
	add_tool(fade_in = new TitleFade(client, this, &client->config.fade_in, x, y + 20));
	x += 100;

	add_tool(fadeout_title = new BC_Title(x, y, "Fade out (sec):"));
	add_tool(fade_out = new TitleFade(client, this, &client->config.fade_out, x, y + 20));
	x += 110;

	add_tool(speed_title = new BC_Title(x, y, "Speed:"));
	speed = new TitleSpeed(client, this, x, y + 20);
	speed->create_objects();
	x += 110;

	add_tool(color_button = new TitleColorButton(client, this, x, y + 20));
	x += 90;
	color_x = x;
	color_y = y + 20;
	color_thread = new TitleColorThread(client, this);

	x = 10;
	y += 50;

	add_tool(text_title = new BC_Title(x, y + 3, "Text:"));

	x += 130;
	add_tool(encoding_title = new BC_Title(x, y + 3, "Encoding:"));
	encoding = new TitleEncoding(client, this, x + 80, y);
	encoding->create_objects();

	x += 280;
	add_tool(timecode = new TitleTimecode(client, x, y));


	x = 10;
	y += 30;
	text = new TitleText(client, 
		this, 
		x, 
		y, 
		get_w() - x - 10, 
		get_h() - y - 20 - 10);
	text->create_objects();

	update_color();

	show_window();
	flush();
	return 0;
}

int TitleWindow::resize_event(int w, int h)
{
	client->window_w = w;
	client->window_h = h;

	clear_box(0, 0, w, h);
	font_title->reposition_window(font_title->get_x(), font_title->get_y());
	font->reposition_window(font->get_x(), font->get_y());
	font_tumbler->reposition_window(font_tumbler->get_x(), font_tumbler->get_y());
	x_title->reposition_window(x_title->get_x(), x_title->get_y());
	title_x->reposition_window(title_x->get_x(), title_x->get_y());
	y_title->reposition_window(y_title->get_x(), y_title->get_y());
	title_y->reposition_window(title_y->get_x(), title_y->get_y());
	style_title->reposition_window(style_title->get_x(), style_title->get_y());
	italic->reposition_window(italic->get_x(), italic->get_y());
	bold->reposition_window(bold->get_x(), bold->get_y());
	size_title->reposition_window(size_title->get_x(), size_title->get_y());
	size->reposition_window(size->get_x(), size->get_y());
	encoding_title->reposition_window(encoding_title->get_x(), encoding_title->get_y());
	encoding->reposition_window(encoding->get_x(), encoding->get_y());
	color_button->reposition_window(color_button->get_x(), color_button->get_y());
	motion_title->reposition_window(motion_title->get_x(), motion_title->get_y());
	motion->reposition_window(motion->get_x(), motion->get_y());
	loop->reposition_window(loop->get_x(), loop->get_y());
	dropshadow_title->reposition_window(dropshadow_title->get_x(), dropshadow_title->get_y());
	dropshadow->reposition_window(dropshadow->get_x(), dropshadow->get_y());
	fadein_title->reposition_window(fadein_title->get_x(), fadein_title->get_y());
	fade_in->reposition_window(fade_in->get_x(), fade_in->get_y());
	fadeout_title->reposition_window(fadeout_title->get_x(), fadeout_title->get_y());
	fade_out->reposition_window(fade_out->get_x(), fade_out->get_y());
	text_title->reposition_window(text_title->get_x(), text_title->get_y());
	timecode->reposition_window(timecode->get_x(), timecode->get_y());

	text->reposition_window(text->get_x(), 
		text->get_y(), 
		w - text->get_x() - 10,
		BC_TextBox::pixels_to_rows(this, MEDIUMFONT, h - text->get_y() - 10));



	justify_title->reposition_window(justify_title->get_x(), justify_title->get_y());
	left->reposition_window(left->get_x(), left->get_y());
	center->reposition_window(center->get_x(), center->get_y());
	right->reposition_window(right->get_x(), right->get_y());
	top->reposition_window(top->get_x(), top->get_y());
	mid->reposition_window(mid->get_x(), mid->get_y());
	bottom->reposition_window(bottom->get_x(), bottom->get_y());
	speed_title->reposition_window(speed_title->get_x(), speed_title->get_y());
	speed->reposition_window(speed->get_x(), speed->get_y());
	update_color();
	flash();

	return 1;
}


void TitleWindow::previous_font()
{
	int current_font = font->get_number();
	current_font--;
	if(current_font < 0) current_font = fonts.total - 1;

	if(current_font < 0 || current_font >= fonts.total) return;

	for(int i = 0; i < fonts.total; i++)
	{
		fonts.values[i]->set_selected(i == current_font);
	}

	font->update(fonts.values[current_font]->get_text());
	strcpy(client->config.font, fonts.values[current_font]->get_text());
	client->send_configure_change();
}

void  TitleWindow::next_font()
{
	int current_font = font->get_number();
	current_font++;
	if(current_font >= fonts.total) current_font = 0;

	if(current_font < 0 || current_font >= fonts.total) return;

	for(int i = 0; i < fonts.total; i++)
	{
		fonts.values[i]->set_selected(i == current_font);
	}

	font->update(fonts.values[current_font]->get_text());
	strcpy(client->config.font, fonts.values[current_font]->get_text());
	client->send_configure_change();
}


int TitleWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}

void TitleWindow::update_color()
{
//printf("TitleWindow::update_color %x\n", client->config.color);
	set_color(client->config.color);
	draw_box(color_x, color_y, 100, 30);
	flash(color_x, color_y, 100, 30);
}

void TitleWindow::update_justification()
{
	left->update(client->config.hjustification == JUSTIFY_LEFT);
	center->update(client->config.hjustification == JUSTIFY_CENTER);
	right->update(client->config.hjustification == JUSTIFY_RIGHT);
	top->update(client->config.vjustification == JUSTIFY_TOP);
	mid->update(client->config.vjustification == JUSTIFY_MID);
	bottom->update(client->config.vjustification == JUSTIFY_BOTTOM);
}

void TitleWindow::update()
{
	title_x->update((long)client->config.x);
	title_y->update((long)client->config.y);
	italic->update(client->config.style & FONT_ITALIC);
	bold->update(client->config.style & FONT_BOLD);
	size->update(client->config.size);
	encoding->update(client->config.encoding);
	motion->update(TitleMain::motion_to_text(client->config.motion_strategy));
	loop->update(client->config.loop);
	dropshadow->update((float)client->config.dropshadow);
	fade_in->update((float)client->config.fade_in);
	fade_out->update((float)client->config.fade_out);
	font->update(client->config.font);
	text->update(client->config.text);
	speed->update(client->config.pixels_per_second);
	update_justification();
	update_color();
}


TitleFontTumble::TitleFontTumble(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Tumbler(x, y)
{
	this->client = client;
	this->window = window;
}
int TitleFontTumble::handle_up_event()
{
	window->previous_font();
	return 1;
}

int TitleFontTumble::handle_down_event()
{
	window->next_font();
	return 1;
}

TitleBold::TitleBold(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.style & FONT_BOLD, "Bold")
{
	this->client = client;
	this->window = window;
}

int TitleBold::handle_event()
{
	client->config.style = (client->config.style & ~FONT_BOLD) | (get_value() ? FONT_BOLD : 0);
	client->send_configure_change();
	return 1;
}

TitleItalic::TitleItalic(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.style & FONT_ITALIC, "Italic")
{
	this->client = client;
	this->window = window;
}
int TitleItalic::handle_event()
{
	client->config.style = (client->config.style & ~FONT_ITALIC) | (get_value() ? FONT_ITALIC : 0);
	client->send_configure_change();
	return 1;
}

TitleSize::TitleSize(TitleMain *client, TitleWindow *window, int x, int y, char *text)
 : BC_PopupTextBox(window, 
		&window->sizes,
		text,
		x, 
		y, 
		100,
		300)
{
	this->client = client;
	this->window = window;
}
TitleSize::~TitleSize()
{
}
int TitleSize::handle_event()
{
	client->config.size = atol(get_text());
//printf("TitleSize::handle_event 1 %s\n", get_text());
	client->send_configure_change();
	return 1;
}
void TitleSize::update(int size)
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", size);
	BC_PopupTextBox::update(string);
}
TitleEncoding::TitleEncoding(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_PopupTextBox(window, 
		&window->encodings,
		client->config.encoding,
		x, 
		y, 
		100,
		300)
{
	this->client = client;
	this->window = window;
}

TitleEncoding::~TitleEncoding()
{
}
int TitleEncoding::handle_event()
{
	strcpy(client->config.encoding, get_text());
	client->send_configure_change();
	return 1;
}

TitleColorButton::TitleColorButton(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_GenericButton(x, y, "Color...")
{
	this->client = client;
	this->window = window;
}
int TitleColorButton::handle_event()
{
	window->color_thread->start_window(client->config.color, 0);
	return 1;
}

TitleMotion::TitleMotion(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_PopupTextBox(window, 
		&window->paths,
		client->motion_to_text(client->config.motion_strategy),
		x, 
		y, 
		120,
		100)
{
	this->client = client;
	this->window = window;
}
int TitleMotion::handle_event()
{
	client->config.motion_strategy = client->text_to_motion(get_text());
	client->send_configure_change();
	return 1;
}

TitleLoop::TitleLoop(TitleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.loop, "Loop")
{
	this->client = client;
}
int TitleLoop::handle_event()
{
	client->config.loop = get_value();
	client->send_configure_change();
	return 1;
}

TitleTimecode::TitleTimecode(TitleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.timecode, "Stamp timecode")
{
	this->client = client;
}
int TitleTimecode::handle_event()
{
	client->config.timecode = get_value();
	client->send_configure_change();
	return 1;
}

TitleFade::TitleFade(TitleMain *client, TitleWindow *window, double *value, int x, int y)
 : BC_TextBox(x, y, 90, 1, (float)*value)
{
	this->client = client;
	this->window = window;
	this->value = value;
}

int TitleFade::handle_event()
{
	*value = atof(get_text());
	client->send_configure_change();
	return 1;
}

TitleFont::TitleFont(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_PopupTextBox(window, 
		&window->fonts,
		client->config.font,
		x, 
		y, 
		200,
		500)
{
	this->client = client;
	this->window = window;
}
int TitleFont::handle_event()
{
	strcpy(client->config.font, get_text());
	client->send_configure_change();
	return 1;
}

TitleText::TitleText(TitleMain *client, 
	TitleWindow *window, 
	int x, 
	int y, 
	int w, 
	int h)
 : BC_ScrollTextBox(window, 
		x, 
		y, 
		w,
		BC_TextBox::pixels_to_rows(window, MEDIUMFONT, h),
		client->config.text)
{
	this->client = client;
	this->window = window;
//printf("TitleText::TitleText %s\n", client->config.text);
}

int TitleText::handle_event()
{
	strcpy(client->config.text, get_text());
	client->send_configure_change();
	return 1;
}


TitleDropShadow::TitleDropShadow(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
 	(long)client->config.dropshadow,
	(long)0,
	(long)1000,
	x, 
	y, 
	70)
{
	this->client = client;
	this->window = window;
}
int TitleDropShadow::handle_event()
{
	client->config.dropshadow = atol(get_text());
	client->send_configure_change();
	return 1;
}


TitleX::TitleX(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
 	(long)client->config.x,
	(long)-2048,
	(long)2048,
	x, 
	y, 
	60)
{
	this->client = client;
	this->window = window;
}
int TitleX::handle_event()
{
	client->config.x = atol(get_text());
	client->send_configure_change();
	return 1;
}

TitleY::TitleY(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
 	(long)client->config.y, 
	(long)-2048,
	(long)2048,
	x, 
	y, 
	60)
{
	this->client = client;
	this->window = window;
}
int TitleY::handle_event()
{
	client->config.y = atol(get_text());
	client->send_configure_change();
	return 1;
}


TitleSpeed::TitleSpeed(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
 	(float)client->config.pixels_per_second, 
	(float)0,
	(float)1000,
	x, 
	y, 
	70)
{
	this->client = client;
}


int TitleSpeed::handle_event()
{
	client->config.pixels_per_second = atof(get_text());
	client->send_configure_change();
	return 1;
}







TitleLeft::TitleLeft(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_LEFT, "Left")
{
	this->client = client;
	this->window = window;
}
int TitleLeft::handle_event()
{
	client->config.hjustification = JUSTIFY_LEFT;
	window->update_justification();
	client->send_configure_change();
	return 1;
}

TitleCenter::TitleCenter(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_CENTER, "Center")
{
	this->client = client;
	this->window = window;
}
int TitleCenter::handle_event()
{
	client->config.hjustification = JUSTIFY_CENTER;
	window->update_justification();
	client->send_configure_change();
	return 1;
}

TitleRight::TitleRight(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_RIGHT, "Right")
{
	this->client = client;
	this->window = window;
}
int TitleRight::handle_event()
{
	client->config.hjustification = JUSTIFY_RIGHT;
	window->update_justification();
	client->send_configure_change();
	return 1;
}



TitleTop::TitleTop(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_TOP, "Top")
{
	this->client = client;
	this->window = window;
}
int TitleTop::handle_event()
{
	client->config.vjustification = JUSTIFY_TOP;
	window->update_justification();
	client->send_configure_change();
	return 1;
}

TitleMid::TitleMid(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_MID, "Mid")
{
	this->client = client;
	this->window = window;
}
int TitleMid::handle_event()
{
	client->config.vjustification = JUSTIFY_MID;
	window->update_justification();
	client->send_configure_change();
	return 1;
}

TitleBottom::TitleBottom(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_BOTTOM, "Bottom")
{
	this->client = client;
	this->window = window;
}
int TitleBottom::handle_event()
{
	client->config.vjustification = JUSTIFY_BOTTOM;
	window->update_justification();
	client->send_configure_change();
	return 1;
}



TitleColorThread::TitleColorThread(TitleMain *client, TitleWindow *window)
 : ColorThread()
{
	this->client = client;
	this->window = window;
}

int TitleColorThread::handle_event(int output)
{
	client->config.color = output;
	window->update_color();
	window->flush();
	client->send_configure_change();
	return 1;
}
