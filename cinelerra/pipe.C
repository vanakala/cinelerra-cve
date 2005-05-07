#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "guicast.h"
#include "pipe.h"
#include "defaults.h"
#include "file.h"

extern "C" {
	int pipe_sigpipe_received;

	void pipe_handle_sigpipe(int signum) {
		printf("Received sigpipe\n");
		pipe_sigpipe_received++;
	}
}

Pipe::Pipe(char *command, char *sub_str, char sub_char) { 
	this->command = command;
	this->sub_str = sub_str;
	this->sub_char = sub_char;

	complete[0] = '\0';
	file = NULL;
	fd = -1;

	// FUTURE: could probably set to SIG_IGN once things work
	signal(SIGPIPE, pipe_handle_sigpipe);
}

Pipe::~Pipe() {		
	close();
}

int Pipe::substitute() {
	if (command == NULL) {
		strcpy(complete, "");
		return 0;
	}

	if (sub_str == NULL || sub_char == '\0') {
		strcpy(complete, command);
		return 0;
	}

	int count = 0;
	char *c = command;
	char *f = complete;
	while (*c) {
		// directly copy anything substitution char
		if (*c != sub_char) {
			*f++ = *c++;
			continue;
		}
		
		// move over the substitution character
		c++;

		// two substitution characters in a row is literal
		if (*c == sub_char) {
			*f++ = *c++;
			continue;
		}

		// insert the file string at the substitution point
		if (f + strlen(sub_str) - complete > sizeof(complete)) {
			printf("Pipe::substitute(): max length exceeded\n");
			return -1;
		}
		strcpy(f, sub_str);
		f += strlen(sub_str);
		count++;
	}

	return count;
}
	
	

int Pipe::open(char *mode) {
	if (file) close();

	if (mode == NULL) {
		printf("Pipe::open(): no mode given\n");
		return 1;
	}

	if (substitute() < 0) {
		return 1;
	}

	if (complete == NULL || strlen(complete) == 0) {
		printf("Pipe::open(): no pipe to open\n");
		return 1;
	}

	printf("trying popen(%s)\n", complete);
	file = popen(complete, mode);
	if (file != NULL) {
		fd = fileno(file);
		return 0;
	}

	// NOTE: popen() fails only if fork/exec fails
	//       there is no immediate way to see if command failed
	//       As such, one must expect to raise SIGPIPE on failure
	printf("Pipe::open(%s,%s) failed: %s\n", 
	       complete, mode, strerror(errno));
	return 1;
}	

int Pipe::open_read() {
	return open("r");
}

int Pipe::open_write() {
	return open("w");
}

void Pipe::close() {
	pclose(file);
	file = 0;
	fd = -1;
}


PipeConfig::PipeConfig(BC_WindowBase *window, Defaults *defaults, Asset *asset)
{
	this->window = window;
	this->defaults = defaults;
	this->asset = asset;
}

// NOTE: Default destructor should destroy all subwindows

int PipeConfig::create_objects(int x, int y, int textbox_width, int format) {
	// NOTE: out of order so pipe_textbox is available to pipe_checkbox
	textbox = new BC_TextBox(x + 120, y, textbox_width, 1, asset->pipe);
	window->add_subwindow(textbox);
	if (! asset->use_pipe) textbox->disable();

	window->add_subwindow(new BC_Title(x, y, _("Use Pipe:")));
	checkbox = new PipeCheckBox(85, y - 5, asset->use_pipe, textbox);
	window->add_subwindow(checkbox);

	x += 120;	
	x += textbox_width;

	recent = new BC_RecentList("PIPE", defaults, textbox,
				10, x, y, 350, 100);
	window->add_subwindow(recent);
	recent->load_items(FILE_FORMAT_PREFIX(format));
}

PipeCheckBox::PipeCheckBox(int x, int y, int value, BC_TextBox *textbox)
	: BC_CheckBox(x, y, value)
{
	this->textbox = textbox;
}
int PipeCheckBox::handle_event() {
	if (get_value()) textbox->enable();
	else textbox->disable();
}

PipeStatus::PipeStatus(int x, int y, char *default_string) 
	: BC_Title(x, y, default_string)
{
	this->default_string = default_string;
}
int PipeStatus::set_status(Asset *asset) {
	if (! asset->use_pipe || ! asset->pipe || ! *(asset->pipe)) {
		set_color(BLACK);
		sprintf(status, default_string);
	}
	else {
		set_color(RED);
		sprintf(status, "| %s", asset->pipe);
	}

	update(status);
}



PipePreset::PipePreset(int x, int y, char *title, PipeConfig *config)
	: BC_PopupMenu(x, y, 150, title)
{
	this->config = config;
	this->title = title;
}

int PipePreset::handle_event() {
	char *text = get_text();
	// NOTE: preset items must have a '|' before the actual command
	char *pipe = strchr(text, '|');
	// pipe + 1 to skip over the '|'
	if (pipe) config->textbox->update(pipe + 1);

	config->textbox->enable();
	config->checkbox->set_value(1, 1);
	
	// menuitem sets the title after selection but we reset it
	set_text(title);
}
