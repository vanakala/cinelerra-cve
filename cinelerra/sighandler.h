#ifndef SIGHANDLER_H
#define SIGHANDLER_H

#include <signal.h>

class SigHandler
{
private:
  static void* entrypoint();
protected:
  virtual void signal_handler() = 0;
public:
	SigHandler(int signal_);
	~SigHandler();
};

#endif
