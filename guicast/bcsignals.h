#ifndef BCSIGNALS_H
#define BCSIGNALS_H

class BC_Signals
{
public:
	BC_Signals();
	virtual void signal_handler(int signum);
};

#endif
