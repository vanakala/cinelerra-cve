#ifndef RENDERFARMFSSERVER_H
#define RENDERFARMFSSERVER_H

#include "renderfarm.inc"
#include "renderfarmfsserver.inc"

class RenderFarmFSServer
{
public:
	RenderFarmFSServer(RenderFarmServerThread *server);
	~RenderFarmFSServer();

	void initialize();
	int handle_request(int request_id, int request_size, char *buffer);

	RenderFarmServerThread *server;
};






#endif
