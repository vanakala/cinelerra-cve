#include "bcdisplayinfo.h"
#include "clip.h"
#include "svgwin.h"
#include "string.h"
#include "filexml.h"
#include <unistd.h>
#include <fcntl.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#include "empty_svg.h"



PLUGIN_THREAD_OBJECT(SvgMain, SvgThread, SvgWin)






SvgWin::SvgWin(SvgMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x,
	y,
	300, 
	280, 
	300, 
	280, 
	0, 
	0,
	1)
{ 
	this->client = client; 
}

SvgWin::~SvgWin()
{
}

int SvgWin::create_objects()
{
	int x = 10, y = 10;

//	add_tool(new BC_Title(x, y, _("In X:")));
	y += 20;
//	in_x = new SvgCoord(this, client, x, y, &client->config.in_x);
//	in_x->create_objects();
	y += 30;

//	add_tool(new BC_Title(x, y, _("In Y:")));
	y += 20;
//	in_y = new SvgCoord(this, client, x, y, &client->config.in_y);
//	in_y->create_objects();
	y += 30;

//	add_tool(new BC_Title(x, y, _("In W:")));
	y += 20;
//	in_w = new SvgCoord(this, client, x, y, &client->config.in_w);
//	in_w->create_objects();
	y += 30;

//	add_tool(new BC_Title(x, y, _("In H:")));
	y += 20;
//	in_h = new SvgCoord(this, client, x, y, &client->config.in_h);
//	in_h->create_objects();
	y += 30;


	x += 150;
	y = 10;
	add_tool(new BC_Title(x, y, _("Out X:")));
	y += 20;
	out_x = new SvgCoord(this, client, x, y, &client->config.out_x);
	out_x->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("Out Y:")));
	y += 20;
	out_y = new SvgCoord(this, client, x, y, &client->config.out_y);
	out_y->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("Out W:")));
	y += 20;
	out_w = new SvgCoord(this, client, x, y, &client->config.out_w);
	out_w->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("Out H:")));
	y += 20;
	out_h = new SvgCoord(this, client, x, y, &client->config.out_h);
	out_h->create_objects();
	y += 30;

	x -= 150;
	add_tool(new_svg_button = new NewSvgButton(client, this, x, y));
	add_tool(edit_svg_button = new EditSvgButton(client, this, x+190, y));
	add_tool(svg_file_title = new BC_Title(x, y+26, client->config.svg_file));

	x +=150;

	show_window();
	flush();
	return 0;
}

int SvgWin::close_event()
{
	set_done(1);
	return 1;
}

SvgCoord::SvgCoord(SvgWin *win, 
	SvgMain *client, 
	int x, 
	int y,
	float *value)
 : BC_TumbleTextBox(win,
 	*value,
	(float)0,
	(float)3000,
	x, 
	y, 
	100)
{
//printf("SvgWidth::SvgWidth %f\n", client->config.w);
	this->client = client;
	this->win = win;
	this->value = value;
}

SvgCoord::~SvgCoord()
{
}

int SvgCoord::handle_event()
{
	*value = atof(get_text());

	client->send_configure_change();
	return 1;
}

NewSvgButton::NewSvgButton(SvgMain *client, SvgWin *window, int x, int y)
 : BC_GenericButton(x, y, _("New/Open SVG..."))
{
	this->client = client;
	this->window = window;
	quit_now = 0;
}
int NewSvgButton::handle_event()
{
	quit_now = 0;
	start();
	return 1;
}

void NewSvgButton::run()
{
// ======================================= get path from user
	int result;
//printf("NewSvgButton::run 1\n");
	char directory[1024], filename[1024];
	sprintf(directory, "~");
	client->defaults->get("DIRECTORY", directory);

// Loop until file is chosen
	do{
		NewSvgWindow *new_window;

		new_window = new NewSvgWindow(client, window, directory);
		new_window->create_objects();
		new_window->update_filter("*.svg");
		result = new_window->run_window();
		client->defaults->update("DIRECTORY", new_window->get_path());
		strcpy(filename, new_window->get_path());
		delete new_window;

// Extend the filename with .svg
		if(strlen(filename) < 4 || 
			strcasecmp(&filename[strlen(filename) - 4], ".svg"))
		{
			strcat(filename, ".svg");
		}

// ======================================= try to save it
		if(filename[0] == 0) return;              // no filename given
		if(result == 1) return;          // user cancelled
		FILE *in;
		if(in = fopen(filename, "rb"))
		{
			fclose(in);
			// file exists
			result = 0; 
		} else
		{
			// create fresh file
			in = fopen(filename, "w+");
			unsigned long size;
			size = (((unsigned long)empty_svg[0]) << 24) +
				(((unsigned long)empty_svg[1]) << 16) +
				(((unsigned long)empty_svg[2]) << 8) +
				((unsigned long)empty_svg[3]);
			printf("in: %p size: %li\n", in, size);
			// FIXME this is not cool on different arhitectures
			
			fwrite(empty_svg+4, size,  1, in);
			fclose(in);
		}
	} while(result);        // file doesn't exist so repeat
	
	// find out starting pixel width and height of the svg
	// FIXME - currenlty works only for mm units
	{
		FileXML xml_file;
		int result;
		xml_file.read_from_file(filename);
		do {
			result = xml_file.read_tag();
//			printf("tag_title: %s\n", xml_file.tag.get_title());
			if (xml_file.tag.title_is("svg\n"))
			{
				char *wvalue, *hvalue;
				int slen;
				float w, h;
				wvalue = xml_file.tag.get_property("width");
				slen = strlen(wvalue);
				if (slen<3 || strcmp(wvalue+slen-2, "mm"))
				{
					printf(_("SVG Width is currently only supported in milimeters: %s, falling back to 100\n"), wvalue);
					w = 100;
				} else {
					wvalue[slen-2] = 0;
					w = atof(wvalue);					
				}
	
				hvalue = xml_file.tag.get_property("height");
				slen = strlen(hvalue);
				if (slen<3 || strcmp(hvalue+slen-2, "mm"))
				{
					printf(_("SVG Height is currently only supported in milimeters: %s, falling back to 100\n"), hvalue);
					h = 100;
				} else {
					hvalue[slen-2] = 0;
					h = atof(hvalue);					
				}
				client->config.in_w = w;
				client->config.in_h = h;
				client->config.out_w = w;
				client->config.out_h = h;
				client->config.in_x = 0;
				client->config.in_y = 0;
//				client->config.out_x = 0;
//				client->config.out_y = 0;

				window->out_w->update(wvalue);
				window->out_h->update(hvalue);
 

			}
		} while (!result);		
	}

	window->svg_file_title->update(filename);
	window->flush();
	strcpy(client->config.svg_file, filename);
	client->need_reconfigure = 1;
	client->force_png_render = 1;
	client->send_configure_change();

// save it
	if(quit_now) window->set_done(0);
	return;
}

EditSvgButton::EditSvgButton(SvgMain *client, SvgWin *window, int x, int y)
 : BC_GenericButton(x, y, _("Edit"))
{
	this->client = client;
	this->window = window;
	this->editing = 0;
}
int EditSvgButton::handle_event()
{
	
	editing_lock.lock();
	if (!editing && client->config.svg_file[0] != 0) 
	{
		editing = 1;
		editing_lock.unlock();
		start();
	} else
	{
		editing_lock.unlock();
	}
		return 1;
}

void EditSvgButton::run()
{
// ======================================= get path from user
	Timer pausetimer;
	long delay;
	int result;
	time_t last_update;
	struct stat st_png;
	char filename_png[1024];
	SvgSodipodiThread *sodipodi_thread = new SvgSodipodiThread(client, window);
	
	strcpy(filename_png, client->config.svg_file);
	strcat(filename_png, ".png");
	result = stat (filename_png, &st_png);
	last_update = st_png.st_mtime;
	if (result) 
		last_update = 0;	

	sodipodi_thread->start();
	while (sodipodi_thread->running()) {
		pausetimer.delay(200); // poll file every 200ms
		struct stat st_png;
		result = stat (filename_png, &st_png);
//		printf("checking: %s %li\n", filename_png, st_png.st_mtime);
		// Check if PNG is newer then what we have in memory
		if (last_update < st_png.st_mtime) {
			// UPDATE IMAGE
//			printf("updated\n");
			client->send_configure_change();
			last_update = st_png.st_mtime;
		}
		
	}
	sodipodi_thread->join();
	editing_lock.lock();
	editing = 0;
	editing_lock.unlock();

}

SvgSodipodiThread::SvgSodipodiThread(SvgMain *client, SvgWin *window)
 : Thread(1)
{
	this->client = client;
	this->window = window;
}

SvgSodipodiThread::~SvgSodipodiThread()
{
	// what do we do? kill sodipodi?
	cancel();
}

void SvgSodipodiThread::run()
{
// Runs the sodipodi
	char command[1024];
	char filename_png[1024];
	strcpy(filename_png, client->config.svg_file);
	strcat(filename_png, ".png");

	sprintf(command, "sodipodi --cinelerra-export-file=%s --export-width=%i --export-height=%i %s",
		filename_png, (int)client->config.in_w, (int)client->config.in_h, client->config.svg_file);
	printf(_("Running external SVG editor: %s\n"), command);		
	enable_cancel();
	system(command);
	disable_cancel();
	int result;
	return;
}



NewSvgWindow::NewSvgWindow(SvgMain *client, SvgWin *window, char *init_directory)
 : BC_FileBox(0,
 	BC_WindowBase::get_resources()->filebox_h / 2,
 	init_directory, 
	_("SVG Plugin: Pick SVG file"), 
	_("Open an existing SVG file or create a new one"))
{ 
	this->window = window; 
}

NewSvgWindow::~NewSvgWindow() {}







