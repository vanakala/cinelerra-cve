#include "mainprogress.h"
#include "reverseaudio.h"

#include <stdio.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


PluginClient* new_plugin(PluginServer *server)
{
	return new ReverseAudio(server);
}

ReverseAudio::ReverseAudio(PluginServer *server)
 : PluginAClient(server)
{
}

ReverseAudio::~ReverseAudio()
{
}

char* ReverseAudio::plugin_title() { return _("Reverse audio"); }

int ReverseAudio::is_realtime() { return 0; }

VFrame* ReverseAudio::new_picon()
{
	return 0;
}

int ReverseAudio::start_loop()
{
//printf("ReverseAudio::start_loop 1\n");
	if(PluginClient::interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, 
			(PluginClient::end - PluginClient::start));
	}

	current_position = PluginClient::end;
	total_written = 0;
//printf("ReverseAudio::start_loop 2\n");
	return 0;
}


int ReverseAudio::stop_loop()
{
	if(PluginClient::interactive)
	{
		progress->stop_progress();
		delete progress;
	}
	return 0;
}

int ReverseAudio::process_loop(double *buffer, int64_t &write_length)
{
	int result = 0;
	int64_t fragment_len;
//printf("ReverseAudio::process_loop 1\n");

	fragment_len = PluginClient::in_buffer_size;
//printf("ReverseAudio::process_loop 2\n");
	if(current_position - fragment_len < PluginClient::start)
	{
		fragment_len = current_position - PluginClient::start;
		result = 1;
	}
//printf("ReverseAudio::process_loop 3 %d %d\n", current_position, fragment_len);

	read_samples(buffer, current_position - fragment_len, fragment_len);

//printf("ReverseAudio::process_loop 4 %p\n", buffer);
	for(int j = 0; j < fragment_len / 2; j++)
	{
		double sample = buffer[j];
		buffer[j] = buffer[fragment_len - 1 - j];
		buffer[fragment_len - 1 -j] = sample;
	}
//printf("ReverseAudio::process_loop 5 %d\n", current_position);

	current_position -= fragment_len;
//printf("ReverseAudio::process_loop 6 %d\n", current_position);
	total_written += fragment_len;
	write_length = fragment_len;
	if(PluginClient::interactive) result = progress->update(total_written);
//printf("ReverseAudio::process_loop 7 %d\n", current_position);

	return result;
}
