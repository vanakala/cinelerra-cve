#include "mainprogress.h"
#include "reversevideo.h"






PluginClient* new_plugin(PluginServer *server)
{
	return new ReverseVideo(server);
}










ReverseVideo::ReverseVideo(PluginServer *server)
 : PluginVClient(server)
{
	current_position = -1;
}


ReverseVideo::~ReverseVideo()
{
}

char* ReverseVideo::plugin_title()
{
	return "Reverse video";
}

VFrame* ReverseVideo::new_picon()
{
	return 0;
}

int ReverseVideo::start_loop()
{
	if(PluginClient::interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, 
			(PluginClient::end - PluginClient::start));
	}

	current_position = PluginClient::end - 1;
	return 0;
}


int ReverseVideo::stop_loop()
{
	if(PluginClient::interactive)
	{
		progress->stop_progress();
		delete progress;
	}
	return 0;
}

int ReverseVideo::process_loop(VFrame *buffer)
{
//printf("ReverseVideo::process_loop 1\n");
	int result = 0;
//printf("ReverseVideo::process_loop 1\n");
	
	read_frame(buffer, current_position);
//printf("ReverseVideo::process_loop 2\n");
	
	current_position--;
	
	
	if(PluginClient::interactive) 
		result = progress->update(PluginClient::end - current_position);
	
	if(current_position < PluginClient::start) result = 1;
	
	return result;
}







