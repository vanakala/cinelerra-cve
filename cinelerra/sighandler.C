#include "sighandler.h"

SigHandler::SigHandler(int signal_)
{
	signal(signal_, SigHandler::entrypoint);
}

SigHandler::~SigHandler()
{
}

void* SigHandler::entrypoint(void *parameters)
{
	SigHandler *pt = static_cast<SigHandler *>(parameters);
	
	pt->signal_handler();
}
